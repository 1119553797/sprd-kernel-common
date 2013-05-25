/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/math64.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/io.h>//for ioremap
#include <linux/pid.h>

#include <mach/hardware.h>

#include "gsp_drv.h"
#include "scaler_coef_cal.h"

#ifdef GSP_WORK_AROUND1
#include <linux/dma-mapping.h>
#endif

static volatile pid_t			gsp_cur_client_pid = INVALID_USER_ID;
static struct semaphore         gsp_hw_resource_sem;//cnt == 1,only one thread can access critical section at the same time
static struct semaphore         gsp_wait_interrupt_sem;// init to 0, gsp done/timeout/client discard release sem
GSP_CONFIG_INFO_T				s_gsp_cfg = {0};//protect by gsp_hw_resource_sem
static struct proc_dir_entry    *gsp_drv_proc_file;


static gsp_user gsp_user_array[GSP_MAX_USER]= {0};
static gsp_user* gsp_get_user(pid_t user_pid)
{
    gsp_user* ret_user = NULL;
    int i;

    for (i = 0; i < GSP_MAX_USER; i ++)
    {
        if ((gsp_user_array + i)->pid == user_pid)
        {
            ret_user = gsp_user_array + i;
            break;
        }
    }

    if (ret_user == NULL)
    {
        for (i = 0; i < GSP_MAX_USER; i ++)
        {
            if ((gsp_user_array + i)->pid == INVALID_USER_ID)
            {
                ret_user = gsp_user_array + i;
                ret_user->pid = user_pid;
                break;
            }
        }
    }

    return ret_user;
}

#ifdef GSP_WORK_AROUND1
const int32_t g_half_pi_cos[8]= {0,0/*-270*/,-1/*-180*/,0/*-90*/,1/*0*/,0/*90*/,-1/*180*/,0/*270*/};
#define half_pi_cos(r)  (g_half_pi_cos[(r)+4])
const int32_t g_half_pi_sin[8]= {0,1/*-270*/,0/*-180*/,-1/*-90*/,0/*0*/,1/*90*/,0/*180*/,-1/*270*/};
#define half_pi_sin(r)  (g_half_pi_sin[(r)+4])

/*
func:GSP_point_in_layer0(x,y)
desc:check if (x,y)point is in des-layer0 area
return:1 in , 0 not
caution:s_gsp_cfg must be set before calling this procedure
*/
static uint32_t GSP_point_in_layer0(uint32_t x,uint32_t y)
{
    GSP_RECT_T des_rect0;
    des_rect0 = s_gsp_cfg.layer0_info.des_rect;

    if(des_rect0.st_x <= x
       && des_rect0.st_y <= y
       && (des_rect0.st_x+des_rect0.rect_w-1) >= x
       && (des_rect0.st_y+des_rect0.rect_h-1) >= y)
    {
        return 1;
    }
    return 0;
}

/*
func:GSP_point_in_layer1(x,y)
desc:check if (x,y)point is in des-layer1 area
return:1 in , 0 not
caution:s_gsp_cfg must be set before calling this procedure
*/
static uint32_t GSP_point_in_layer1(uint32_t x,uint32_t y)
{
    GSP_RECT_T des_rect1;

    des_rect1.st_x = s_gsp_cfg.layer1_info.des_pos.pos_pt_x;
    des_rect1.st_y = s_gsp_cfg.layer1_info.des_pos.pos_pt_y;
    if(s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270_M)
    {
        des_rect1.rect_w = s_gsp_cfg.layer1_info.clip_rect.rect_h;
        des_rect1.rect_h = s_gsp_cfg.layer1_info.clip_rect.rect_w;
    }
    else
    {
        des_rect1.rect_w = s_gsp_cfg.layer1_info.clip_rect.rect_w;
        des_rect1.rect_h = s_gsp_cfg.layer1_info.clip_rect.rect_h;
    }

    if(des_rect1.st_x <= x
       && des_rect1.st_y <= y
       && (des_rect1.st_x+des_rect1.rect_w-1) >= x
       && (des_rect1.st_y+des_rect1.rect_h-1) >= y)
    {
        return 1;
    }
    return 0;
}


/*
func:GSP_get_points_in_layer0(void)
desc:called by GSP_work_around1() to get the points-count of L1 in L0
return:the point count
caution:s_gsp_cfg must be set before calling this procedure
*/
static uint32_t GSP_get_points_in_layer0(void)
{
    uint32_t cnt = 0;
    GSP_RECT_T des_rect1;


    des_rect1.st_x = s_gsp_cfg.layer1_info.des_pos.pos_pt_x;
    des_rect1.st_y = s_gsp_cfg.layer1_info.des_pos.pos_pt_y;
    des_rect1.rect_w = s_gsp_cfg.layer1_info.clip_rect.rect_w;
    des_rect1.rect_h = s_gsp_cfg.layer1_info.clip_rect.rect_h;

    cnt += GSP_point_in_layer0(des_rect1.st_x,des_rect1.st_y);
    cnt += GSP_point_in_layer0(des_rect1.st_x+des_rect1.rect_w-1,des_rect1.st_y);
    cnt += GSP_point_in_layer0(des_rect1.st_x,des_rect1.st_y+des_rect1.rect_h-1);
    cnt += GSP_point_in_layer0(des_rect1.st_x+des_rect1.rect_w-1,des_rect1.st_y+des_rect1.rect_h-1);
    return cnt;
}
/*
func:GSP_get_points_in_layer0(void)
desc:called by GSP_work_around1() to get the point count of L0 in L1
return:the point count
caution:s_gsp_cfg must be set before calling this procedure
*/
static uint32_t GSP_get_points_in_layer1(void)
{
    uint32_t cnt = 0;
    GSP_RECT_T des_rect0;
    des_rect0 = s_gsp_cfg.layer0_info.des_rect;

    cnt += GSP_point_in_layer1(des_rect0.st_x,des_rect0.st_y);
    cnt += GSP_point_in_layer1(des_rect0.st_x+des_rect0.rect_w-1,des_rect0.st_y);
    cnt += GSP_point_in_layer1(des_rect0.st_x,des_rect0.st_y+des_rect0.rect_h-1);
    cnt += GSP_point_in_layer1(des_rect0.st_x+des_rect0.rect_w-1,des_rect0.st_y+des_rect0.rect_h-1);
    return cnt;
}

/*
func:GSP_layer0_layer1_overlap()
desc:check they overlap or not in des layer
return:1 overlap;0 not
*/
static uint32_t GSP_layer0_layer1_overlap(void)
{
    GSP_RECT_T des_rect0;
    GSP_RECT_T des_rect1;

    des_rect0 = s_gsp_cfg.layer0_info.des_rect;
    des_rect1.st_x = s_gsp_cfg.layer1_info.des_pos.pos_pt_x;
    des_rect1.st_y = s_gsp_cfg.layer1_info.des_pos.pos_pt_y;
    des_rect1.rect_w = s_gsp_cfg.layer1_info.clip_rect.rect_w;
    des_rect1.rect_h = s_gsp_cfg.layer1_info.clip_rect.rect_h;
    if(s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270_M)
    {
        des_rect1.rect_w = s_gsp_cfg.layer1_info.clip_rect.rect_h;
        des_rect1.rect_h = s_gsp_cfg.layer1_info.clip_rect.rect_w;
    }
    else
    {
        des_rect1.rect_w = s_gsp_cfg.layer1_info.clip_rect.rect_w;
        des_rect1.rect_h = s_gsp_cfg.layer1_info.clip_rect.rect_h;
    }

    //L1 (st_x,st_y) in C F G H I area
    if(des_rect1.st_x > (des_rect0.st_x + des_rect0.rect_w - 1)
       ||des_rect1.st_y > (des_rect0.st_y + des_rect0.rect_h - 1))
    {
        return 0;
    }

    //L1 (st_x,st_y) in A area
    if((des_rect1.st_x < des_rect0.st_x) && (des_rect1.st_y < des_rect0.st_y))
    {
        if(((des_rect1.st_x+des_rect1.rect_w-1) <= des_rect0.st_x)||((des_rect1.st_y+des_rect1.rect_h-1) <= des_rect0.st_y))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }

    //L1 (st_x,st_y) in B area
    if((des_rect1.st_x >= des_rect0.st_x) && (des_rect1.st_y < des_rect0.st_y))
    {
        if((des_rect1.st_y+des_rect1.rect_h-1) < des_rect0.st_y)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }

    //L1 (st_x,st_y) in D area
    if((des_rect1.st_x < des_rect0.st_x) && (des_rect1.st_y >= des_rect0.st_y))
    {
        if((des_rect1.st_x+des_rect1.rect_w-1) < des_rect0.st_x)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }

    //L1 (st_x,st_y) in E area
    return 1;
}




static void Matrix3x3SetIdentity(Matrix33 m)
{
    uint32_t r = 0;//row
    uint32_t c = 0;//column

    for(r=0; r<3; r++)
    {
        for(c=0; c<3; c++)
        {
            m[r][c]= (r==c);
        }
    }
}

/*
func:MatrixMul33
desc:multiply m1 times m2, store result in m2
*/
static void MatrixMul33(Matrix33 m1,Matrix33 m2)
{
    uint32_t r = 0;//row
    uint32_t c = 0;//column
    Matrix33 tm;//temp matrix

    for(r=0; r<3; r++)
    {
        for(c=0; c<3; c++)
        {
            tm[r][c]=m1[r][0]*m2[0][c]+m1[r][1]*m2[1][c]+m1[r][2]*m2[2][c];
        }
    }

    for(r=0; r<3; r++)
    {
        for(c=0; c<3; c++)
        {
            m2[r][c]=tm[r][c];
        }
    }
}

/*
func:MatrixMul31
desc:multiply m1 times m2, store result in m2
*/

static void MatrixMul31(Matrix33 m1,Matrix31 m2)
{
    uint32_t r = 0;//row
    uint32_t i = 0;
    Matrix31 tm;//temp matrix

    for(r=0; r<3; r++)
    {
        tm[r]=m1[r][0]*m2[0]+m1[r][1]*m2[1]+m1[r][2]*m2[2];
    }

    for(r=0; r<3; r++)
    {
        m2[r]=tm[r];
    }
}


/*
func:GSP_Gen_CMDQ()
desc:split L1 des image into a few parts according to their relationship, each parts will be bitblt by GSP separately in CMDQ mode
     so, each part's src start point and width/height should be calculated out, then fill the CMDQ descriptor
return: the number of parts should be excuted in CMDQ
*/
static uint32_t GSP_Gen_CMDQ(GSP_LAYER1_REG_T *pDescriptors_walk,GSP_L1_L0_RELATIONSHIP_E relation)
{
    uint32_t part_cnt = 0;//
    uint32_t i = 0;//when L0 in L1,split L1 into 5 parts,
    int32_t am_flag = 1;//need anti-mirror
    uint32_t L1_des_w = 0;//L1 des width after rotation
    uint32_t L1_des_h = 0;//L1 des width after rotation
    int32_t L1_anti_rotate = 0;
    int32_t L1_top_left_x = 4095;
    int32_t L1_top_left_y = 4095;
    GSP_RECT_T rect;
    GSP_POS_PT_T pos;

    PART_POINTS L0_des_Points = {0};
    PART_POINTS L1_des_Points = {0};
    PART_POINTS parts_Points_des[PARTS_CNT_MAX] = {0};//L1 des parts not overlap
    PART_POINTS parts_Points_src[PARTS_CNT_MAX] = {0};//L1 des parts in clip pic

#ifdef TRANSLATION_CALC_OPT
    PART_POINTS parts_Points_des_co[PARTS_CNT_MAX] = {0};//coordinat point at (parts_Points_des[0].st_x,parts_Points_des[0].st_y)
    PART_POINTS parts_Points_des_am[PARTS_CNT_MAX] = {0};//anti-mirror
    PART_POINTS parts_Points_des_ar[PARTS_CNT_MAX] = {0};//anti-ratation
#else

    PART_POINTS parts_Points_des_ar_matrix[PARTS_CNT_MAX] = {0};//anti-ratation
    PART_POINTS parts_Points_src_matrix[PARTS_CNT_MAX] = {0};//L1 des parts in clip pic

    Matrix33 matrix_t;//translate to origin at (parts_Points_des[0].st_x,parts_Points_des[0].st_y)
    Matrix33 matrix_am;//anti-mirror
    Matrix33 matrix_ar;//anti-rotation
    Matrix33 matrix_cmp;//composition of matrix_t & matrix_am & matrix_ar
    Matrix33 matrix_map_t;//
    Matrix31 m_point0 = {0,0,1};
    Matrix31 m_point1 = {0,0,1};

    Matrix3x3SetIdentity(matrix_t);
    Matrix3x3SetIdentity(matrix_am);
    Matrix3x3SetIdentity(matrix_ar);
    Matrix3x3SetIdentity(matrix_cmp);
    Matrix3x3SetIdentity(matrix_map_t);
#endif



    if(s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270_M)
    {
        L1_des_w = s_gsp_cfg.layer1_info.clip_rect.rect_h;
        L1_des_h = s_gsp_cfg.layer1_info.clip_rect.rect_w;

    }
    else
    {
        L1_des_w = s_gsp_cfg.layer1_info.clip_rect.rect_w;
        L1_des_h = s_gsp_cfg.layer1_info.clip_rect.rect_h;
    }


    L0_des_Points.st_x = s_gsp_cfg.layer0_info.des_rect.st_x;
    L0_des_Points.st_y = s_gsp_cfg.layer0_info.des_rect.st_y;
    L0_des_Points.end_x = L0_des_Points.st_x + s_gsp_cfg.layer0_info.des_rect.rect_w - 1;
    L0_des_Points.end_y = L0_des_Points.st_y + s_gsp_cfg.layer0_info.des_rect.rect_h - 1;

    L1_des_Points.st_x = s_gsp_cfg.layer1_info.des_pos.pos_pt_x;
    L1_des_Points.st_y = s_gsp_cfg.layer1_info.des_pos.pos_pt_y;
    L1_des_Points.end_x = L1_des_Points.st_x + L1_des_w - 1;
    L1_des_Points.end_y = L1_des_Points.st_y + L1_des_h - 1;

    //split des-layer0 area into parts
    switch(relation)
    {
        case L1_L0_RELATIONSHIP_AQ:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.st_y - 1;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.end_y + 1;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.st_x - 1;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.end_x + 1;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            i++;
            parts_Points_des[i] = L0_des_Points;//add calc overlap-part's clip_rect in src
            part_cnt = i;
        }
        break;
        case L1_L0_RELATIONSHIP_AD:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.st_y - 1;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.st_x - 1;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            part_cnt = i;
        }
        break;
        case L1_L0_RELATIONSHIP_AC:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.st_y - 1;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.st_x - 1;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.end_x + 1;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            part_cnt = i;
        }
        break;
        case L1_L0_RELATIONSHIP_AO:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.st_y - 1;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.st_x - 1;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.end_y + 1;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_BP:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.st_y - 1;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y + 1;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_CQ:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.st_y - 1;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.end_x + 1;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.end_y + 1;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_CF:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.st_y - 1;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.end_x + 1;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_BE:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.st_y - 1;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_GK:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.st_x - 1;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.end_x + 1;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_OQ:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.st_x - 1;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.end_x + 1;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.end_y + 1;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_LO:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.st_x - 1;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.end_y + 1;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_GH:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.st_x - 1;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L0_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_NQ:
        {
            i = 0;
            parts_Points_des[i].st_x = L0_des_Points.end_x + 1;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.end_y + 1;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_JK:
        {
            i = 0;
            parts_Points_des[i].st_x = L0_des_Points.end_x + 1;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L0_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            part_cnt = i;

        }
        break;
        case L1_L0_RELATIONSHIP_MP:
        {
            i = 0;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L0_des_Points.end_y + 1;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L1_des_Points.end_y;

            i++;
            parts_Points_des[i].st_x = L1_des_Points.st_x;
            parts_Points_des[i].st_y = L1_des_Points.st_y;
            parts_Points_des[i].end_x = L1_des_Points.end_x;
            parts_Points_des[i].end_y = L0_des_Points.end_y;

            part_cnt = i;

        }
        break;
        default:
        {

        }
        break;
    }
    part_cnt = i+1;

#ifdef TRANSLATION_CALC_OPT
    //transform to origin point at (parts_Points_des[0].st_x,parts_Points_des[0].st_y)
    i = 0;
    while(i < part_cnt)
    {
        parts_Points_des_co[i].st_x = parts_Points_des[i].st_x - L1_des_Points.st_x;
        parts_Points_des_co[i].st_y = parts_Points_des[i].st_y - L1_des_Points.st_y;
        parts_Points_des_co[i].end_x = parts_Points_des[i].end_x - L1_des_Points.st_x;
        parts_Points_des_co[i].end_y = parts_Points_des[i].end_y - L1_des_Points.st_y;
        i++;
    }

    //do anti-mirror
    if(s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_0_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_180_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270_M)
    {
        am_flag = -1;
    }
    i = 0;
    while(i < part_cnt)
    {
        parts_Points_des_am[i].st_x = parts_Points_des_co[i].st_x;
        parts_Points_des_am[i].st_y = am_flag * parts_Points_des_co[i].st_y;
        parts_Points_des_am[i].end_x = parts_Points_des_co[i].end_x;
        parts_Points_des_am[i].end_y = am_flag * parts_Points_des_co[i].end_y;
        i++;
    }

    //do anti-ratation
    i = 0;
    L1_anti_rotate = s_gsp_cfg.layer1_info.rot_angle;
    L1_anti_rotate &= 0x3;
    while(i < part_cnt)
    {
        parts_Points_des_ar[i].st_x = parts_Points_des_am[i].st_x * half_pi_cos(L1_anti_rotate) - parts_Points_des_am[i].st_y * half_pi_sin(L1_anti_rotate);
        parts_Points_des_ar[i].st_y = parts_Points_des_am[i].st_x * half_pi_sin(L1_anti_rotate) + parts_Points_des_am[i].st_y * half_pi_cos(L1_anti_rotate);
        parts_Points_des_ar[i].end_x = parts_Points_des_am[i].end_x * half_pi_cos(L1_anti_rotate) - parts_Points_des_am[i].end_y * half_pi_sin(L1_anti_rotate);
        parts_Points_des_ar[i].end_y = parts_Points_des_am[i].end_x * half_pi_sin(L1_anti_rotate) + parts_Points_des_am[i].end_y * half_pi_cos(L1_anti_rotate);

        //get the left-top corner point coordinate in parts_Points_des_ar[]
        L1_top_left_x = (L1_top_left_x > parts_Points_des_ar[i].st_x)?parts_Points_des_ar[i].st_x:L1_top_left_x;
        L1_top_left_x = (L1_top_left_x > parts_Points_des_ar[i].end_x)?parts_Points_des_ar[i].end_x:L1_top_left_x;
        L1_top_left_y = (L1_top_left_y > parts_Points_des_ar[i].st_y)?parts_Points_des_ar[i].st_y:L1_top_left_y;
        L1_top_left_y = (L1_top_left_y > parts_Points_des_ar[i].end_y)?parts_Points_des_ar[i].end_y:L1_top_left_y;

        i++;
    }

    //
    i = 0;
    while(i < part_cnt)
    {
        parts_Points_src[i].st_x = parts_Points_des_ar[i].st_x - L1_top_left_x + s_gsp_cfg.layer1_info.clip_rect.st_x;
        parts_Points_src[i].st_y = parts_Points_des_ar[i].st_y - L1_top_left_y + s_gsp_cfg.layer1_info.clip_rect.st_y;
        parts_Points_src[i].end_x = parts_Points_des_ar[i].end_x - L1_top_left_x + s_gsp_cfg.layer1_info.clip_rect.st_x;
        parts_Points_src[i].end_y = parts_Points_des_ar[i].end_y - L1_top_left_y + s_gsp_cfg.layer1_info.clip_rect.st_y;
        i++;
    }

    i = 0;
    while(i < part_cnt)
    {
        GSP_GET_RECT_FROM_PART_POINTS(parts_Points_src[i],rect);
        GSP_L1_CLIPRECT_SET(rect);
        pos.pos_pt_x = parts_Points_des[i].st_x;
        pos.pos_pt_y = parts_Points_des[i].st_y;
        GSP_L1_DESPOS_SET(pos);

        pDescriptors_walk[i] = *(volatile GSP_LAYER1_REG_T *)GSP_L1_BASE;
        i++;
    }
#else
    matrix_t[0][2] = -s_gsp_cfg.layer1_info.des_pos.pos_pt_x;
    matrix_t[1][2] = -s_gsp_cfg.layer1_info.des_pos.pos_pt_y;

    if(s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_0_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_180_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270_M)
    {
        matrix_am[1][1] = -1;
    }

    L1_anti_rotate = s_gsp_cfg.layer1_info.rot_angle;
    L1_anti_rotate &= 0x3;
    matrix_ar[0][0] = half_pi_cos(L1_anti_rotate);
    matrix_ar[0][1] = -half_pi_sin(L1_anti_rotate);
    matrix_ar[1][1] = half_pi_cos(L1_anti_rotate);
    matrix_ar[1][0] = matrix_ar[0][1];

    MatrixMul33(matrix_t, matrix_cmp);
    MatrixMul33(matrix_am, matrix_cmp);
    MatrixMul33(matrix_ar, matrix_cmp);

    i = 0;
    while(i < part_cnt)
    {
        m_point0[0] = parts_Points_des[i].st_x;
        m_point0[1] = parts_Points_des[i].st_y;
        m_point1[0] = parts_Points_des[i].end_x;
        m_point1[1] = parts_Points_des[i].end_y;


        MatrixMul31(matrix_cmp, m_point0);
        parts_Points_des_ar_matrix[i].st_x = m_point0[0];
        parts_Points_des_ar_matrix[i].st_y = m_point0[1];
        MatrixMul31(matrix_cmp, m_point1);
        parts_Points_des_ar_matrix[i].end_x = m_point1[0];
        parts_Points_des_ar_matrix[i].end_y = m_point1[1];

        L1_top_left_x = MIN(L1_top_left_x, MIN(parts_Points_des_ar_matrix[i].st_x, parts_Points_des_ar_matrix[i].end_x));
        L1_top_left_y = MIN(L1_top_left_y, MIN(parts_Points_des_ar_matrix[i].st_y, parts_Points_des_ar_matrix[i].end_y));
        i++;
    }

    //
    matrix_map_t[0][2] = - L1_top_left_x + s_gsp_cfg.layer1_info.clip_rect.st_x;
    matrix_map_t[1][2] = - L1_top_left_y + s_gsp_cfg.layer1_info.clip_rect.st_y;

    i = 0;
    while(i < part_cnt)
    {
        m_point0[0] = parts_Points_des_ar_matrix[i].st_x;
        m_point0[1] = parts_Points_des_ar_matrix[i].st_y;
        m_point1[0] = parts_Points_des_ar_matrix[i].end_x;
        m_point1[1] = parts_Points_des_ar_matrix[i].end_y;

        MatrixMul31(matrix_map_t,m_point0);
        parts_Points_src_matrix[i].st_x = m_point0[0];
        parts_Points_src_matrix[i].st_y = m_point0[1];
        MatrixMul31(matrix_map_t, m_point1);
        parts_Points_src_matrix[i].end_x = m_point1[0];
        parts_Points_src_matrix[i].end_y = m_point1[1];
        i++;
    }
    i = 0;
    while(i < part_cnt)
    {
        GSP_GET_RECT_FROM_PART_POINTS(parts_Points_src_matrix[i],rect);
        GSP_L1_CLIPRECT_SET(rect);
        pos.pos_pt_x = parts_Points_des[i].st_x;
        pos.pos_pt_y = parts_Points_des[i].st_y;
        GSP_L1_DESPOS_SET(pos);

        pDescriptors_walk[i] = *(volatile GSP_LAYER1_REG_T *)GSP_L1_BASE;
        i++;
    }
#endif
    return part_cnt-1;
}


/*
CMDQ is a special-GSP-mode, designed to blend small L1 area with Ld, so GSP will read Ld image data,
but in GSP chip logic, only L0 L1 channel can read memory, then set Ld 3 plane base addr to L0 base addr can resolve the problem
of cource, pitch and format should change to Ld style.
the width of L0 clip_rect&des_rect should be equal the Ld_pitch, height should be larger than any situation,so set to 4095.

if the two rect is not set properly, GSP state machine will work run, only one CMD in CMDQ will be excuted.
if the rect width is not set properly, GSP will generate err code or code will hold on "busy"
if the addr diff with Ld, some task in CMDQ will generate wrong image.
*/
#define L0_CFG_PREPARE_FOR_CMDQ()\
{\
    GSP_RECT_T clip_rect = {0,0,0,4095};\
    clip_rect.rect_w = s_gsp_cfg.layer_des_info.pitch;\
    GSP_L0_CLIPRECT_SET(clip_rect);\
    GSP_L0_DESRECT_SET(clip_rect);\
    GSP_L0_ADDR_SET(s_gsp_cfg.layer_des_info.src_addr);\
    GSP_L0_PITCH_SET(s_gsp_cfg.layer_des_info.pitch);\
    GSP_L0_IMGFORMAT_SET(s_gsp_cfg.layer_des_info.img_format);\
    GSP_L0_ENABLE_SET(0)


#define L0_CFG_RESTORE_FROM_CMDQ()\
    GSP_L0_ADDR_SET(s_gsp_cfg.layer0_info.src_addr);\
    GSP_L0_PITCH_SET(s_gsp_cfg.layer0_info.pitch);\
    GSP_L0_CLIPRECT_SET(s_gsp_cfg.layer0_info.clip_rect);\
    GSP_L0_DESRECT_SET(s_gsp_cfg.layer0_info.des_rect);\
    GSP_L0_IMGFORMAT_SET(s_gsp_cfg.layer0_info.img_format);\
    GSP_L0_ENABLE_SET(s_gsp_cfg.layer0_info.layer_en);\
}

/*
func:GSP_work_around1()
desc:when GSP dell with the following case, GSP will not stop
L0-1280x720 :from(0,0)  crop 1280x720 do(scaling rotation) to (0,0)     600x600
L1-1280x720 :from(300,0)crop  600x600 do(rotation)         to (300,100) 600x600
Ld-1280x720 :

the problem need 2 prerequisite:
1 scaling down to smaller than half of source width or height
2 L1 beyond L0 area

work-around method:split L1 area into many rectangle parts,one part in L0 area, do another parts first,then do the in part
so the trigger-ioctl call will take much more time than before
*/
static void GSP_work_around1(gsp_user* pUserdata)
{
    uint32_t L1inL0cnt=0;
    uint32_t L0inL1cnt=0;
    int32_t ret = 0;
    GSP_L1_L0_RELATIONSHIP_E relation = L1_L0_RELATIONSHIP_I;
    uint32_t L1_des_w = 0;//L1 des width after rotation
    uint32_t L1_des_h = 0;//L1 des width after rotation
    PART_POINTS L0_des_Points = {0};
    PART_POINTS L1_des_Points = {0};
    uint32_t descriptors_byte_len = 0;
    GSP_LAYER1_REG_T *pDescriptors = NULL;//vitrual address of descriptors of Layer1 bitblt CMDQ
    dma_addr_t      Descriptors_pa = 0;//physic address of descriptors, points to the same memory with pDescriptors
    GSP_CMDQ_REG_T CmdQCfg = {0};//CMDQ config register structer

    //only blending possibly has this problem
    if(s_gsp_cfg.layer1_info.layer_en == 0
       ||s_gsp_cfg.layer0_info.layer_en == 0)
    {
        return;
    }

    if(s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_90_M
       ||s_gsp_cfg.layer1_info.rot_angle == GSP_ROT_ANGLE_270_M)
    {
        L1_des_w = s_gsp_cfg.layer1_info.clip_rect.rect_h;
        L1_des_h = s_gsp_cfg.layer1_info.clip_rect.rect_w;

    }
    else
    {
        L1_des_w = s_gsp_cfg.layer1_info.clip_rect.rect_w;
        L1_des_h = s_gsp_cfg.layer1_info.clip_rect.rect_h;
    }


    L0_des_Points.st_x = s_gsp_cfg.layer0_info.des_rect.st_x;
    L0_des_Points.st_y = s_gsp_cfg.layer0_info.des_rect.st_y;
    L0_des_Points.end_x = L0_des_Points.st_x + s_gsp_cfg.layer0_info.des_rect.rect_w - 1;
    L0_des_Points.end_y = L0_des_Points.st_y + s_gsp_cfg.layer0_info.des_rect.rect_h - 1;

    L1_des_Points.st_x = s_gsp_cfg.layer1_info.des_pos.pos_pt_x;
    L1_des_Points.st_y = s_gsp_cfg.layer1_info.des_pos.pos_pt_y;
    L1_des_Points.end_x = L1_des_Points.st_x + L1_des_w - 1;
    L1_des_Points.end_y = L1_des_Points.st_y + L1_des_h - 1;


    //check have 1/2 scaling down, if not exit just return
    if(!(s_gsp_cfg.layer0_info.clip_rect.rect_w >= s_gsp_cfg.layer0_info.des_rect.rect_w*2
         ||s_gsp_cfg.layer0_info.clip_rect.rect_h >= s_gsp_cfg.layer0_info.des_rect.rect_h*2))
    {
        return;
    }
    //if they not overlap,its ok,just return
    if(GSP_layer0_layer1_overlap() == 0)
    {
        return;
    }

    //if L1 in L0,its ok,just return
    L1inL0cnt = GSP_get_points_in_layer0();
    if(L1inL0cnt == 4)
    {
        return;
    }

    //they cross overlap, must be processed specially!!
    //get the relationship of the two layer in des layer
    L0inL1cnt = GSP_get_points_in_layer1();
    if(L1_des_Points.st_x < L0_des_Points.st_x && L1_des_Points.st_y < L0_des_Points.st_y)//L1 start point in A
    {
        if(L1inL0cnt == 1)
        {
            relation = L1_L0_RELATIONSHIP_AD;
        }
        else if(L0inL1cnt == 4)
        {
            relation = L1_L0_RELATIONSHIP_AQ;
        }
        else if(L1_des_Points.end_x > L0_des_Points.end_x)
        {
            relation = L1_L0_RELATIONSHIP_AC;
        }
        else
        {
            relation = L1_L0_RELATIONSHIP_AO;
        }

    }
    else if(L1_des_Points.st_x >= L0_des_Points.st_x && L1_des_Points.st_x <= L0_des_Points.end_x
            && L1_des_Points.st_y < L0_des_Points.st_y)//L1 start point in B
    {
        if(L1inL0cnt == 0)
        {
            if(L0inL1cnt == 0)
            {
                relation = L1_L0_RELATIONSHIP_BP;
            }
            else
            {
                relation = L1_L0_RELATIONSHIP_CQ;
            }
        }
        else if(L1inL0cnt == 1)
        {
            relation = L1_L0_RELATIONSHIP_CF;
        }
        else if(L1inL0cnt == 2)
        {
            relation = L1_L0_RELATIONSHIP_BE;
        }
    }
    else if(L1_des_Points.st_y >= L0_des_Points.st_y && L1_des_Points.st_y <= L0_des_Points.end_y
            && L1_des_Points.st_x < L0_des_Points.st_x)//L1 start point in G
    {
        if(L1inL0cnt == 0)
        {
            if(L0inL1cnt == 0)
            {
                relation = L1_L0_RELATIONSHIP_GK;
            }
            else
            {
                relation = L1_L0_RELATIONSHIP_OQ;
            }
        }
        else if(L1inL0cnt == 1)
        {
            relation = L1_L0_RELATIONSHIP_LO;
        }
        else if(L1inL0cnt == 2)
        {
            relation = L1_L0_RELATIONSHIP_GH;
        }
    }
    else//L1 start point in L0
    {
        if(L1inL0cnt == 1)
        {
            relation = L1_L0_RELATIONSHIP_NQ;
        }
        else if(L1_des_Points.end_x > L0_des_Points.end_x)
        {
            relation = L1_L0_RELATIONSHIP_JK;
        }
        else
        {
            relation = L1_L0_RELATIONSHIP_MP;
        }
    }


    //alloc no-cache and no-buffer descriptor memory for CMDQ
    descriptors_byte_len = sizeof(GSP_LAYER1_REG_T) * CMDQ_DPT_DEPTH;
    pDescriptors = (GSP_LAYER1_REG_T *)dma_alloc_coherent(NULL,
                   descriptors_byte_len,
                   &Descriptors_pa,
                   GFP_KERNEL|GFP_DMA);
    if (!pDescriptors)
    {
        printk("gsp_drv_ioctl:pid:0x%08x, can't alloc CMDQ descriptor memory!! just disable L1 !! L%d \n",pUserdata->pid,__LINE__);
        GSP_L1_ENABLE_SET(0);
        return;
    }

    (*(uint32_t*)&CmdQCfg.gsp_cmd_addr_u) = (uint32_t)Descriptors_pa;
    CmdQCfg.gsp_cmd_cfg_u.mBits.cmd_num = GSP_Gen_CMDQ(pDescriptors,relation);
    CmdQCfg.gsp_cmd_cfg_u.mBits.cmd_en = 1;

    //disable L0,and trigger GSP do process the 4 parts
    L0_CFG_PREPARE_FOR_CMDQ();
    GSP_L1_CMDQ_SET(CmdQCfg);

    ret = GSP_Trigger();
    GSP_TRACE("%s:pid:0x%08x, trigger %s!, L%d \n",__func__,pUserdata->pid,(ret)?"failed":"success",__LINE__);
    if(ret)
    {
        printk("error %s:pid:0x%08x, disable Layer1 !!!!!! L%d \n",__func__,pUserdata->pid,__LINE__);
        GSP_L1_ENABLE_SET(0);
        goto exit;
    }

    //ret = down_interruptible(&gsp_wait_interrupt_sem);//interrupt lose
    ret = down_timeout(&gsp_wait_interrupt_sem,10);//for interrupt lose, timeout return -ETIME,
    if (ret == 0)//gsp process over
    {
        GSP_TRACE("%s:pid:0x%08x, wait done sema success, L%d \n",__func__,pUserdata->pid,__LINE__);
    }
    else if (ret == -ETIME)
    {
        printk("%s:pid:0x%08x, wait done sema 10-jiffies-timeout,it's abnormal!!!!!!!! L%d \n",__func__,pUserdata->pid,__LINE__);
        ret = -ERESTARTSYS;
    }
    else if (ret)// == -EINTR
    {
        printk("%s:pid:0x%08x, wait done sema interrupted by a signal, L%d \n",__func__,pUserdata->pid,__LINE__);
        ret = -ERESTARTSYS;
    }

    GSP_Wait_Finish();//wait busy-bit down
    GSP_IRQSTATUS_CLEAR();
    GSP_IRQENABLE_SET(GSP_IRQ_TYPE_DISABLE);
    sema_init(&gsp_wait_interrupt_sem,0);

exit:
    CmdQCfg.gsp_cmd_cfg_u.mBits.cmd_en = 0;//disable CMDQ
    GSP_L1_CMDQ_SET(CmdQCfg);

    L0_CFG_RESTORE_FROM_CMDQ();

    //cfg L1 rect and des_pos as the overlap part
    //though the L1 rect and des_pos has modified in end of GSP_Gen_CMDQ(), but it will be overwriten during CMDQ executing
    *(volatile GSP_LAYER1_REG_T *)GSP_L1_BASE = pDescriptors[CmdQCfg.gsp_cmd_cfg_u.mBits.cmd_num];

    //free descriptor buffer
    dma_free_coherent(NULL,
                      descriptors_byte_len,
                      pDescriptors,
                      Descriptors_pa);

}
#endif


static int32_t gsp_drv_open(struct inode *node, struct file *file)
{
    int32_t ret = -EBUSY;
    gsp_user *pUserdata = NULL;

    GSP_TRACE("gsp_drv_open:pid:0x%08x enter.\n",current->pid);

    pUserdata = (gsp_user *)gsp_get_user(current->pid);

    if (NULL == pUserdata)
    {
        GSP_TRACE("gsp_drv_open:pid:0x%08x user cnt full.\n",current->pid);
        goto exit;
    }
    GSP_TRACE("gsp_drv_open:pid:0x%08x bf wait open sema.\n",current->pid);
    ret = down_interruptible(&pUserdata->sem_open);
    if(!ret)
    {
        GSP_TRACE("gsp_drv_open:pid:0x%08x  wait open sema success.\n",current->pid);
        file->private_data = pUserdata;
        ret = 0;
    }
    else
    {
        GSP_TRACE("gsp_drv_open:pid:0x%08x  wait open sema failed.\n",current->pid);
    }

exit:
    return ret;
}



static int32_t gsp_drv_release(struct inode *node, struct file *file)
{
    gsp_user *pUserdata = file->private_data;

    //GSP_TRACE("gsp_drv_release \n");
    GSP_TRACE("gsp_drv_release:pid:0x%08x.\n\n",current->pid);
    if(pUserdata == NULL)
    {
        printk("gsp_drv_release:error--pUserdata is null!, pid-0x%08x \n\n",current->pid);
        return -ENODEV;
    }
    pUserdata->pid = INVALID_USER_ID;
    sema_init(&pUserdata->sem_open, 1);
/*
    //if caller thread hold gsp_hw_resource_sem,but was terminated,we release hw semaphore here
    if(gsp_cur_client_pid == current->pid)
    {
        GSP_Wait_Finish();//wait busy-bit down
        GSP_Deinit();
        //pUserdata->own_gsp_flag = 0;
        gsp_cur_client_pid = INVALID_USER_ID;
        sema_init(&gsp_wait_interrupt_sem,0);
        GSP_TRACE("%s:pid:0x%08x, release gsp-hw sema, L%d \n",__func__,pUserdata->pid,__LINE__);
        up(&gsp_hw_resource_sem);
    }
*/
    file->private_data = NULL;

    return 0;
}


ssize_t gsp_drv_write(struct file *file, const char __user * u_data, size_t cnt, loff_t *cnt_ret)
{
    gsp_user* pUserdata = file->private_data;
    if(pUserdata == NULL)
    {
        printk("%s:error--pUserdata is null!, pid-0x%08x \n\n",__func__,current->pid);
        return -ENODEV;
    }

    //GSP_TRACE("gsp_drv_write %d, \n", cnt);
    GSP_TRACE("gsp_drv_write:pid:0x%08x.\n",current->pid);

    pUserdata->is_exit_force = 1;

    //current->pid
    //gsp_cur_client_pid
    //pUserdata->pid
    /*
    nomater the target thread "pUserdata->pid" wait on done-sema or hw resource sema,
    send a signal to resume it and make it return from ioctl(),we does not up a sema to make target
    thread can distinguish GSP done and signal interrupt.
    */
    send_sig(SIGABRT, pUserdata->pid, 0);

    return 1;
}

ssize_t gsp_drv_read(struct file *file, char __user *u_data, size_t cnt, loff_t *cnt_ret)
{
    char rt_word[32]= {0};
    gsp_user* pUserdata = file->private_data;
    if(pUserdata == NULL)
    {
        printk("%s:error--pUserdata is null!, pid-0x%08x \n\n",__func__,current->pid);
        return -ENODEV;
    }

    *cnt_ret = 0;
    *cnt_ret += sprintf(rt_word + *cnt_ret, "gsp read %d\n",cnt);
    return copy_to_user(u_data, (void*)rt_word, (uint32_t)*cnt_ret);
}


static void GSP_Coef_Tap_Convert(uint8_t h_tap,uint8_t v_tap)
{
    switch(h_tap)
    {
        case 8:
            s_gsp_cfg.layer0_info.row_tap_mode = 0;
            break;

        case 6:
            s_gsp_cfg.layer0_info.row_tap_mode = 1;
            break;

        case 4:
            s_gsp_cfg.layer0_info.row_tap_mode = 2;
            break;

        case 2:
            s_gsp_cfg.layer0_info.row_tap_mode = 3;
            break;

        default:
            s_gsp_cfg.layer0_info.row_tap_mode = 0;
            break;
    }

    switch(v_tap)
    {
        case 8:
            s_gsp_cfg.layer0_info.col_tap_mode = 0;
            break;

        case 6:
            s_gsp_cfg.layer0_info.col_tap_mode = 1;
            break;

        case 4:
            s_gsp_cfg.layer0_info.col_tap_mode = 2;
            break;

        case 2:
            s_gsp_cfg.layer0_info.col_tap_mode = 3;
            break;

        default:
            s_gsp_cfg.layer0_info.col_tap_mode = 0;
            break;
    }
}


static void GSP_Scaling_Coef_Gen_And_Config(void)
{
    uint8_t     h_tap = 8;
    uint8_t     v_tap = 8;
    uint32_t    *tmp_buf = NULL;
    uint32_t    *h_coeff = NULL;
    uint32_t    *v_coeff = NULL;
    volatile uint32_t    coef_factor_w = 0;
    volatile uint32_t    coef_factor_h = 0;
    volatile uint32_t    after_rotate_w = 0;
    volatile uint32_t    after_rotate_h = 0;
    volatile uint32_t    coef_in_w = 0;
    volatile uint32_t    coef_in_h = 0;
    volatile uint32_t    coef_out_w = 0;
    volatile uint32_t    coef_out_h = 0;

    if((s_gsp_cfg.layer0_info.clip_rect.rect_w != s_gsp_cfg.layer0_info.des_rect.rect_w) ||
       (s_gsp_cfg.layer0_info.clip_rect.rect_h != s_gsp_cfg.layer0_info.des_rect.rect_h))
    {
        s_gsp_cfg.layer0_info.scaling_en = 1;
    }

    if(s_gsp_cfg.layer0_info.scaling_en == 1)
    {
        if(s_gsp_cfg.layer0_info.des_rect.rect_w < 4
           ||s_gsp_cfg.layer0_info.des_rect.rect_h < 4)
        {
            return;
        }

        if(s_gsp_cfg.layer0_info.rot_angle == GSP_ROT_ANGLE_0
           ||s_gsp_cfg.layer0_info.rot_angle == GSP_ROT_ANGLE_180
           ||s_gsp_cfg.layer0_info.rot_angle == GSP_ROT_ANGLE_0_M
           ||s_gsp_cfg.layer0_info.rot_angle == GSP_ROT_ANGLE_180_M)
        {
            after_rotate_w = s_gsp_cfg.layer0_info.clip_rect.rect_w;
            after_rotate_h = s_gsp_cfg.layer0_info.clip_rect.rect_h;
        }
        else if(s_gsp_cfg.layer0_info.rot_angle == GSP_ROT_ANGLE_90
                ||s_gsp_cfg.layer0_info.rot_angle == GSP_ROT_ANGLE_270
                ||s_gsp_cfg.layer0_info.rot_angle == GSP_ROT_ANGLE_90_M
                ||s_gsp_cfg.layer0_info.rot_angle == GSP_ROT_ANGLE_270_M)
        {
            after_rotate_w = s_gsp_cfg.layer0_info.clip_rect.rect_h;
            after_rotate_h = s_gsp_cfg.layer0_info.clip_rect.rect_w;
        }

        coef_factor_w = CEIL(after_rotate_w,s_gsp_cfg.layer0_info.des_rect.rect_w);
        coef_factor_h = CEIL(after_rotate_h,s_gsp_cfg.layer0_info.des_rect.rect_h);

        if(coef_factor_w > 16 || coef_factor_h > 16)
        {
            return;
        }

        if(coef_factor_w > 8)
        {
            coef_factor_w = 4;
        }
        else if(coef_factor_w > 4)
        {
            coef_factor_w = 2;
        }
        else
        {
            coef_factor_w = 1;
        }

        if(coef_factor_h > 8)
        {
            coef_factor_h = 4;
        }
        else if(coef_factor_h > 4)
        {
            coef_factor_h = 2;
        }
        else
        {
            coef_factor_h= 1;
        }

        tmp_buf = (uint32_t *)kmalloc(GSP_COEFF_BUF_SIZE, GFP_KERNEL);
        if (NULL == tmp_buf)
        {
            printk("SCALE DRV: No mem to alloc coeff buffer! \n");
            return;//SCALE_RTN_NO_MEM
        }
        h_coeff = tmp_buf;
        v_coeff = tmp_buf + (GSP_COEFF_COEF_SIZE/4);

        coef_in_w = CEIL(after_rotate_w,coef_factor_w);
        coef_in_h = CEIL(after_rotate_h,coef_factor_h);
        coef_out_w = s_gsp_cfg.layer0_info.des_rect.rect_w;
        coef_out_h = s_gsp_cfg.layer0_info.des_rect.rect_h;

        if (!(GSP_Gen_Block_Ccaler_Coef(coef_in_w,
                                        coef_in_h,
                                        coef_out_w,
                                        coef_out_h,
                                        h_coeff,
                                        v_coeff,
                                        tmp_buf + (GSP_COEFF_COEF_SIZE/2),
                                        GSP_COEFF_POOL_SIZE)))
        {
            kfree(tmp_buf);
            printk("GSP DRV: GSP_Gen_Block_Ccaler_Coef error! \n");
            return;
        }

        GSP_Scale_Coef_Tab_Config(h_coeff,v_coeff);//write coef-metrix to register
        GSP_Coef_Tap_Convert(h_tap,v_tap);
        kfree(tmp_buf);
    }
}

/*
func:GSP_Info_Config
desc:config info to register
*/
static uint32_t GSP_Info_Config(void)
{
    //check GSP is in idle
    if(GSP_WORKSTATUS_GET())
    {
        GSP_ASSERT();
    }

    GSP_ConfigLayer(GSP_MODULE_LAYER0);
    GSP_ConfigLayer(GSP_MODULE_LAYER1);
    GSP_ConfigLayer(GSP_MODULE_ID_MAX);
    GSP_ConfigLayer(GSP_MODULE_DST);
    return GSP_ERRCODE_GET();
}

/*
func:GSP_Cache_Flush
desc:
*/
static void GSP_Cache_Flush(void)
{

}

/*
func:GSP_Cache_Invalidate
desc:
*/
static void GSP_Cache_Invalidate(void)
{

}

/*
func:GSP_Release_HWSema
desc:
*/
static void GSP_Release_HWSema(void)
{
    gsp_user *pTempUserdata = NULL;
    GSP_TRACE("%s:pid:0x%08x, was killed without release GSP hw semaphore, L%d \n",__func__,gsp_cur_client_pid,__LINE__);

    pTempUserdata = (gsp_user *)gsp_get_user(gsp_cur_client_pid);
    pTempUserdata->pid = INVALID_USER_ID;
    sema_init(&pTempUserdata->sem_open, 1);

    GSP_Wait_Finish();//wait busy-bit down
    GSP_Deinit();
    gsp_cur_client_pid = INVALID_USER_ID;
    sema_init(&gsp_wait_interrupt_sem,0);
    up(&gsp_hw_resource_sem);
}


static long gsp_drv_ioctl(struct file *file,
                          uint32_t cmd,
                          unsigned long arg)
{
    int32_t ret = -GSP_NO_ERR;
    uint32_t param_size = _IOC_SIZE(cmd);
    gsp_user* pUserdata = file->private_data;
    if(pUserdata == NULL)
    {
        printk("%s:error--pUserdata is null!, pid-0x%08x \n\n",__func__,current->pid);
        return -ENODEV;
    }

    GSP_TRACE("%s:pid:0x%08x, io number 0x%x, param_size %d \n",
              __func__,
              pUserdata->pid,
              _IOC_NR(cmd),
              param_size);

    switch (cmd)
    {
        case GSP_IO_SET_PARAM:
        {
            if (param_size)
            {
                GSP_TRACE("%s:pid:0x%08x, bf wait gsp-hw sema, L%d \n",__func__,pUserdata->pid,__LINE__);

                // the caller thread was killed without release GSP hw semaphore
                if(gsp_cur_client_pid != INVALID_USER_ID)
                {
                    volatile struct pid * __pid = NULL;
                    volatile struct task_struct *__task = NULL;

                    GSP_TRACE("%sL%d current:%08x store_pid:0x%08x, \n",__func__,__LINE__,current->pid,gsp_cur_client_pid);
                    barrier();
                    __pid = find_get_pid(gsp_cur_client_pid);
                    if(__pid != NULL)
                    {
                        __task = get_pid_task(__pid,PIDTYPE_PID);
                        barrier();

                        if(__task != NULL)
                        {
                            if(__task->pid != gsp_cur_client_pid)
                            {
                                GSP_Release_HWSema();
                            }
                        }
                        else
                        {
                            GSP_Release_HWSema();
                        }
                    }
                    else
                    {
                        GSP_Release_HWSema();
                    }
                    barrier();
                }


                ret = down_interruptible(&gsp_hw_resource_sem);
                if(ret)
                {
                    GSP_TRACE("%s:pid:0x%08x, wait gsp-hw sema interrupt by signal,return, L%d \n",__func__,pUserdata->pid,__LINE__);
                    //receive a signal
                    ret = -ERESTARTSYS;
                    goto exit;
                }
                GSP_TRACE("%s:pid:0x%08x, wait gsp-hw sema success, L%d \n",__func__,pUserdata->pid,__LINE__);
                gsp_cur_client_pid = pUserdata->pid;
                ret=copy_from_user((void*)&s_gsp_cfg, (void*)arg, param_size);
                if(ret)
                {
                    printk("%s:pid:0x%08x, copy_params_from_user failed! \n",__func__,pUserdata->pid);
                    ret = -EFAULT;
                    gsp_cur_client_pid = INVALID_USER_ID;
                    up(&gsp_hw_resource_sem);
                    goto exit;
                }
                else
                {
                    GSP_TRACE("%s:pid:0x%08x, copy_params_from_user success!, L%d \n",__func__,pUserdata->pid,__LINE__);
                    GSP_Init();

                    // if the y u v address is virtual, should be converted to phy address here!!!
                    if((s_gsp_cfg.layer0_info.layer_en == 1)
                       &&((s_gsp_cfg.layer0_info.clip_rect.rect_w != s_gsp_cfg.layer0_info.des_rect.rect_w) ||
                          (s_gsp_cfg.layer0_info.clip_rect.rect_h != s_gsp_cfg.layer0_info.des_rect.rect_h)))
                    {
                        s_gsp_cfg.layer0_info.scaling_en = 1;
                    }
                    s_gsp_cfg.misc_info.dithering_en = 1;//dither enabled default

                    ret = GSP_Info_Config();
                    GSP_TRACE("%s:pid:0x%08x, config hw %s!, L%d \n",__func__,pUserdata->pid,(ret>0)?"failed":"success",__LINE__);
                    if(ret)
                    {
                        ret |= 0x80000000;
                        GSP_Deinit();
                        gsp_cur_client_pid = INVALID_USER_ID;
                        up(&gsp_hw_resource_sem);
                        GSP_TRACE("%s:pid:0x%08x, release hw sema, L%d \n",__func__,pUserdata->pid,__LINE__);
                    }
                }
            }
        }
        break;

        case GSP_IO_TRIGGER_RUN:
        {
            GSP_TRACE("%s:pid:0x%08x, in trigger to run , L%d \n",__func__,pUserdata->pid,__LINE__);
            if(gsp_cur_client_pid == pUserdata->pid)
            {
                GSP_TRACE("%s:pid:0x%08x, calc coef and trigger to run , L%d \n",__func__,pUserdata->pid,__LINE__);
                GSP_Cache_Flush();
#ifdef GSP_WORK_AROUND1
                GSP_work_around1(pUserdata);
#endif
                GSP_Scaling_Coef_Gen_And_Config();
                ret = GSP_Trigger();
                GSP_TRACE("%s:pid:0x%08x, trigger %s!, L%d \n",__func__,pUserdata->pid,(ret)?"failed":"success",__LINE__);
                if(ret)
                {
                    GSP_Deinit();

                    gsp_cur_client_pid = INVALID_USER_ID;
                    up(&gsp_hw_resource_sem);
                    GSP_TRACE("%s:pid:0x%08x, release hw sema, L%d \n",__func__,pUserdata->pid,__LINE__);
                }
                else
                {
                }
            }
            else
            {
                GSP_TRACE("%s:pid:0x%08x,exit L%d \n",__func__,pUserdata->pid,__LINE__);
                ret = -EPERM;
                goto exit;
            }
        }
        break;


        case GSP_IO_WAIT_FINISH:
        {
            if(gsp_cur_client_pid == pUserdata->pid)
            {
                GSP_TRACE("%s:pid:0x%08x, bf wait done sema, L%d \n",__func__,pUserdata->pid,__LINE__);
                //ret = down_interruptible(&gsp_wait_interrupt_sem);//interrupt lose
                ret = down_timeout(&gsp_wait_interrupt_sem,10);//for interrupt lose, timeout return -ETIME,
                if (ret == 0)//gsp process over
                {
                    GSP_TRACE("%s:pid:0x%08x, wait done sema success, L%d \n",__func__,pUserdata->pid,__LINE__);
                }
                else if (ret == -ETIME)
                {
                    printk("%s:pid:0x%08x, wait done sema 10-jiffies-timeout,it's abnormal!!!!!!!! L%d \n",__func__,pUserdata->pid,__LINE__);
                    ret = -ERESTARTSYS;
                }
                else if (ret)// == -EINTR
                {
                    printk("%s:pid:0x%08x, wait done sema interrupted by a signal, L%d \n",__func__,pUserdata->pid,__LINE__);
                    ret = -ERESTARTSYS;
                }

                if (pUserdata->is_exit_force)
                {
                    pUserdata->is_exit_force = 0;
                    ret = -1;
                }

                GSP_Wait_Finish();//wait busy-bit down
                GSP_Cache_Invalidate();
                GSP_Deinit();
                gsp_cur_client_pid = INVALID_USER_ID;
                sema_init(&gsp_wait_interrupt_sem,0);
                GSP_TRACE("%s:pid:0x%08x, release gsp-hw sema, L%d \n",__func__,pUserdata->pid,__LINE__);
                up(&gsp_hw_resource_sem);

            }
        }
        break;

        default:
            ret = -ESRCH;
            break;
    }

exit:
    if (ret)
    {
        GSP_TRACE("%s:pid:0x%08x, error code 0x%x \n", __func__,pUserdata->pid,ret);
    }
    return ret;

}



static int32_t  gsp_drv_proc_read(char *page,
                                  char **start,
                                  off_t off,
                                  int32_t count,
                                  int32_t *eof,
                                  void *data)
{
    int32_t len = 0;
    uint32_t i = 0;
    GSP_REG_T *g_gsp_reg =  (GSP_REG_T *)GSP_REG_BASE;

    len += sprintf(page + len, "********************************************* \n");
    len += sprintf(page + len, "%s , u_data_size %d , register :\n",__func__,count);

    len += sprintf(page + len, "misc: run %d|busy %d|errflag %d|errcode %02d|dither %d|pmmod0 %d|pmmod1 %d|pmen %d|scale %d|reserv2 %d|scl_stat_clr %d|l0en %d|l1en %d|rb %d\n",
                   g_gsp_reg->gsp_cfg_u.mBits.gsp_run,
                   g_gsp_reg->gsp_cfg_u.mBits.gsp_busy,
                   g_gsp_reg->gsp_cfg_u.mBits.error_flag,
                   g_gsp_reg->gsp_cfg_u.mBits.error_code,
                   g_gsp_reg->gsp_cfg_u.mBits.dither_en,
                   g_gsp_reg->gsp_cfg_u.mBits.pmargb_mod0,
                   g_gsp_reg->gsp_cfg_u.mBits.pmargb_mod1,
                   g_gsp_reg->gsp_cfg_u.mBits.pmargb_en,
                   g_gsp_reg->gsp_cfg_u.mBits.scale_en,
                   g_gsp_reg->gsp_cfg_u.mBits.reserved2,
                   g_gsp_reg->gsp_cfg_u.mBits.scale_status_clr,
                   g_gsp_reg->gsp_cfg_u.mBits.l0_en,
                   g_gsp_reg->gsp_cfg_u.mBits.l1_en,
                   g_gsp_reg->gsp_cfg_u.mBits.dist_rb);
    len += sprintf(page + len, "misc: inten %d|intmod %d|intclr %d\n",
                   g_gsp_reg->gsp_int_cfg_u.mBits.int_en,
                   g_gsp_reg->gsp_int_cfg_u.mBits.int_mod,
                   g_gsp_reg->gsp_int_cfg_u.mBits.int_clr);


    len += sprintf(page + len, "L0 cfg:fmt %d|rot %d|ck %d|pallet %d|rowtap %d|coltap %d\n",
                   g_gsp_reg->gsp_layer0_cfg_u.mBits.img_format_l0,
                   g_gsp_reg->gsp_layer0_cfg_u.mBits.rot_mod_l0,
                   g_gsp_reg->gsp_layer0_cfg_u.mBits.ck_en_l0,
                   g_gsp_reg->gsp_layer0_cfg_u.mBits.pallet_en_l0,
                   g_gsp_reg->gsp_layer0_cfg_u.mBits.row_tap_mod,
                   g_gsp_reg->gsp_layer0_cfg_u.mBits.col_tap_mod);
    len += sprintf(page + len, "L0 blockalpha %03d, pitch %04d,(%04d,%04d)%04dx%04d => (%04d,%04d)%04dx%04d\n",
                   g_gsp_reg->gsp_layer0_alpha_u.mBits.alpha_l0,
                   g_gsp_reg->gsp_layer0_pitch_u.mBits.pitch0,
                   g_gsp_reg->gsp_layer0_clip_start_u.mBits.clip_start_x_l0,
                   g_gsp_reg->gsp_layer0_clip_start_u.mBits.clip_start_y_l0,
                   g_gsp_reg->gsp_layer0_clip_size_u.mBits.clip_size_x_l0,
                   g_gsp_reg->gsp_layer0_clip_size_u.mBits.clip_size_y_l0,
                   g_gsp_reg->gsp_layer0_des_start_u.mBits.des_start_x_l0,
                   g_gsp_reg->gsp_layer0_des_start_u.mBits.des_start_y_l0,
                   g_gsp_reg->gsp_layer0_des_size_u.mBits.des_size_x_l0,
                   g_gsp_reg->gsp_layer0_des_size_u.mBits.des_size_y_l0);
    len += sprintf(page + len, "L0 Yaddr 0x%08x|Uaddr 0x%08x|Vaddr 0x%08x\n",
                   g_gsp_reg->gsp_layer0_y_addr_u.dwValue,
                   g_gsp_reg->gsp_layer0_uv_addr_u.dwValue,
                   g_gsp_reg->gsp_layer0_va_addr_u.dwValue);
    len += sprintf(page + len, "L0 grey(%03d,%03d,%03d) colorkey(%03d,%03d,%03d)\n",
                   g_gsp_reg->gsp_layer0_grey_rgb_u.mBits.layer0_grey_r,
                   g_gsp_reg->gsp_layer0_grey_rgb_u.mBits.layer0_grey_g,
                   g_gsp_reg->gsp_layer0_grey_rgb_u.mBits.layer0_grey_b,
                   g_gsp_reg->gsp_layer0_ck_u.mBits.ck_r_l0,
                   g_gsp_reg->gsp_layer0_ck_u.mBits.ck_g_l0,
                   g_gsp_reg->gsp_layer0_ck_u.mBits.ck_b_l0);
    len += sprintf(page + len, "L0 endian: y %d|u %d|v %d|rgb %d|alpha %d\n",
                   g_gsp_reg->gsp_layer0_endian_u.mBits.y_endian_mod_l0,
                   g_gsp_reg->gsp_layer0_endian_u.mBits.uv_endian_mod_l0,
                   g_gsp_reg->gsp_layer0_endian_u.mBits.va_endian_mod_l0,
                   g_gsp_reg->gsp_layer0_endian_u.mBits.rgb_swap_mod_l0,
                   g_gsp_reg->gsp_layer0_endian_u.mBits.a_swap_mod_l0);


    len += sprintf(page + len, "L1 cfg:fmt %d|rot %d|ck %d|pallet %d\n",
                   g_gsp_reg->gsp_layer1_cfg_u.mBits.img_format_l1,
                   g_gsp_reg->gsp_layer1_cfg_u.mBits.rot_mod_l1,
                   g_gsp_reg->gsp_layer1_cfg_u.mBits.ck_en_l1,
                   g_gsp_reg->gsp_layer1_cfg_u.mBits.pallet_en_l1);
    len += sprintf(page + len, "L1 blockalpha %03d, pitch %d04,(%04d,%04d)%04dx%04d => (%04d,%04d)\n",
                   g_gsp_reg->gsp_layer1_alpha_u.mBits.alpha_l1,
                   g_gsp_reg->gsp_layer1_pitch_u.mBits.pitch1,
                   g_gsp_reg->gsp_layer1_clip_start_u.mBits.clip_start_x_l1,
                   g_gsp_reg->gsp_layer1_clip_start_u.mBits.clip_start_y_l1,
                   g_gsp_reg->gsp_layer1_clip_size_u.mBits.clip_size_x_l1,
                   g_gsp_reg->gsp_layer1_clip_size_u.mBits.clip_size_y_l1,
                   g_gsp_reg->gsp_layer1_des_start_u.mBits.des_start_x_l1,
                   g_gsp_reg->gsp_layer1_des_start_u.mBits.des_start_y_l1);
    len += sprintf(page + len, "L1 Yaddr 0x%08x|Uaddr 0x%08x|Vaddr 0x%08x\n",
                   g_gsp_reg->gsp_layer1_y_addr_u.dwValue,
                   g_gsp_reg->gsp_layer1_uv_addr_u.dwValue,
                   g_gsp_reg->gsp_layer1_va_addr_u.dwValue);
    len += sprintf(page + len, "L1 grey(%03d,%03d,%03d) colorkey(%03d,%03d,%03d)\n",
                   g_gsp_reg->gsp_layer1_grey_rgb_u.mBits.grey_r_l1,
                   g_gsp_reg->gsp_layer1_grey_rgb_u.mBits.grey_g_l1,
                   g_gsp_reg->gsp_layer1_grey_rgb_u.mBits.grey_b_l1,
                   g_gsp_reg->gsp_layer1_ck_u.mBits.ck_r_l1,
                   g_gsp_reg->gsp_layer1_ck_u.mBits.ck_g_l1,
                   g_gsp_reg->gsp_layer1_ck_u.mBits.ck_b_l1);
    len += sprintf(page + len, "L1 endian: y %d|u %d|v %d|rgb %d|alpha %d\n",
                   g_gsp_reg->gsp_layer1_endian_u.mBits.y_endian_mod_l1,
                   g_gsp_reg->gsp_layer1_endian_u.mBits.uv_endian_mod_l1,
                   g_gsp_reg->gsp_layer1_endian_u.mBits.va_endian_mod_l1,
                   g_gsp_reg->gsp_layer1_endian_u.mBits.rgb_swap_mod_l1,
                   g_gsp_reg->gsp_layer1_endian_u.mBits.a_swap_mod_l1);


    len += sprintf(page + len, "Ld cfg:fmt %d|cmpr8 %d|pitch %04d\n",
                   g_gsp_reg->gsp_des_data_cfg_u.mBits.des_img_format,
                   g_gsp_reg->gsp_des_data_cfg_u.mBits.compress_r8,
                   g_gsp_reg->gsp_des_pitch_u.mBits.des_pitch);
    len += sprintf(page + len, "Ld Yaddr 0x%08x|Uaddr 0x%08x|Vaddr 0x%08x\n",
                   g_gsp_reg->gsp_des_y_addr_u.dwValue,
                   g_gsp_reg->gsp_des_uv_addr_u.dwValue,
                   g_gsp_reg->gsp_des_v_addr_u.dwValue);
    len += sprintf(page + len, "Ld endian: y %d|u %d|v %d|rgb %d|alpha %d\n",
                   g_gsp_reg->gsp_des_data_endian_u.mBits.y_endian_mod,
                   g_gsp_reg->gsp_des_data_endian_u.mBits.uv_endian_mod,
                   g_gsp_reg->gsp_des_data_endian_u.mBits.v_endian_mod,
                   g_gsp_reg->gsp_des_data_endian_u.mBits.rgb_swap_mod,
                   g_gsp_reg->gsp_des_data_endian_u.mBits.a_swap_mod);
    len += sprintf(page + len, "********************************************* \n");

    msleep(10);

    return len;
}


static struct file_operations gsp_drv_fops =
{
    .owner          = THIS_MODULE,
    .open           = gsp_drv_open,
    .write          = gsp_drv_write,
    .read           = gsp_drv_read,
    .unlocked_ioctl = gsp_drv_ioctl,
    .release        = gsp_drv_release
};

static struct miscdevice gsp_drv_dev =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "sprd_gsp",
    .fops = &gsp_drv_fops
};
static irqreturn_t gsp_irq_handler(int32_t irq, void *dev_id)
{
    GSP_TRACE("%s enter!\n",__func__);

    GSP_IRQSTATUS_CLEAR();
    GSP_IRQENABLE_SET(GSP_IRQ_TYPE_DISABLE);
    up(&gsp_wait_interrupt_sem);
    return IRQ_HANDLED;
}

int32_t gsp_drv_probe(struct platform_device *pdev)
{
    int32_t ret = 0;
    int32_t i = 0;

    GSP_TRACE("gsp_probe enter .\n");

    ret = misc_register(&gsp_drv_dev);
    if (ret)
    {
        GSP_TRACE("gsp cannot register miscdev (%d)\n", ret);
        goto exit;
    }


    ret = request_irq(IRQ_GSP_INT,//TB_GSP_INT
                      gsp_irq_handler,
                      0,//IRQF_SHARED
                      "GSP",
                      gsp_irq_handler);

    if (ret)
    {
        printk("could not request irq %d\n", IRQ_GSP_INT);
        goto exit1;
    }

    gsp_drv_proc_file = create_proc_read_entry("driver/sprd_gsp",
                        0444,
                        NULL,
                        gsp_drv_proc_read,
                        NULL);
    if (unlikely(NULL == gsp_drv_proc_file))
    {
        printk("Can't create an entry for gsp in /proc \n");
        ret = ENOMEM;
        goto exit2;
    }


    for (i=0; i<GSP_MAX_USER; i++)
    {
        gsp_user_array[i].pid = INVALID_USER_ID;
        gsp_user_array[i].is_exit_force = 0;
        sema_init(&gsp_user_array[i].sem_open, 1);
    }

    /* initialize locks*/
    sema_init(&gsp_hw_resource_sem, 1);
    sema_init(&gsp_wait_interrupt_sem, 0);

    return ret;

exit2:
    free_irq(IRQ_GSP_INT, gsp_irq_handler);
exit1:
    misc_deregister(&gsp_drv_dev);
exit:
    return ret;
}

static int32_t gsp_drv_remove(struct platform_device *dev)
{
    GSP_TRACE( "gsp_remove called !\n");

    if (gsp_drv_proc_file)
    {
        remove_proc_entry("driver/sprd_gsp", NULL);
    }
    free_irq(IRQ_GSP_INT, gsp_irq_handler);
    misc_deregister(&gsp_drv_dev);
    return 0;
}

static struct platform_driver gsp_drv_driver =
{
    .probe = gsp_drv_probe,
    .remove = gsp_drv_remove,
    .driver =
    {
        .owner = THIS_MODULE,
        .name = "sprd_gsp"
    }
};

int32_t __init gsp_drv_init(void)
{
    printk("gsp_drv_init enter! \n");

    if (platform_driver_register(&gsp_drv_driver) != 0)
    {
        printk("gsp platform driver register Failed! \n");
        return -1;
    }
    else
    {
        GSP_TRACE("gsp platform driver registered successful! \n");
    }
    return 0;
}

void gsp_drv_exit(void)
{
    platform_driver_unregister(&gsp_drv_driver);
    GSP_TRACE("gsp platform driver unregistered! \n");
}

module_init(gsp_drv_init);
module_exit(gsp_drv_exit);

MODULE_DESCRIPTION("GSP Driver");
MODULE_LICENSE("GPL");

