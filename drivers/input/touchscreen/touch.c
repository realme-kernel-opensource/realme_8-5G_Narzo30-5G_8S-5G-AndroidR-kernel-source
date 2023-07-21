/***************************************************
 * File:touch.c
 * VENDOR_EDIT
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd.
 * Description:
 *             tp dev
 * Version:1.0:
 * Date created:2016/09/02
 * TAG: BSP.TP.Init
*/

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>
#include "oppo_touchscreen/tp_devices.h"
#include "oppo_touchscreen/touchpanel_common.h"
#include <soc/oppo/oppo_project.h>
#include <soc/oppo/device_info.h>
#include "touch.h"

#define MAX_LIMIT_DATA_LENGTH         100
extern char *saved_command_line;
int g_tp_dev_vendor = TP_UNKNOWN;
int g_tp_prj_id = 0;
static bool is_tp_type_got_in_match = false;    /*indicate whether the tp type is specified in the dts*/
struct hw_resource *g_hw_res;
/*if can not compile success, please update vendor/oppo_touchsreen*/
struct tp_dev_name tp_dev_names[] = {
     {TP_OFILM, "OFILM"},
     {TP_BIEL, "BIEL"},
     {TP_TRULY, "TRULY"},
     {TP_BOE, "BOE"},
     {TP_G2Y, "G2Y"},
     {TP_TPK, "TPK"},
     {TP_JDI, "JDI"},
     {TP_TIANMA, "TIANMA"},
     {TP_SAMSUNG, "SAMSUNG"},
     {TP_DSJM, "DSJM"},
     {TP_BOE_B8, "BOEB8"},
     {TP_BOE_B3, "BOE"},
     {TP_CDOT,"CDOT"},
     {TP_INNOLUX, "INNOLUX"},
     {TP_HIMAX_DPT, "DPT"},
     {TP_AUO, "AUO"},
     {TP_DEPUTE, "DEPUTE"},
     {TP_HUAXING, "HUAXING"},
     {TP_HLT, "HLT"},
     {TP_DJN, "DJN"},
     {TP_INX, "INX"},
     {TP_UNKNOWN, "UNKNOWN"},
};

typedef enum {
    TP_INDEX_NULL,
    himax_83112a,
    himax_83112f,
    ili9881_auo,
    ili9881_tm,
    nt36525b_boe,
    nt36525b_inx,
    ili9882n_cdot,
    ili9882n_hlt,
    ili9882n_inx,
    nt36525b_hlt,
    nt36672c,
    ili9881_inx,
    goodix_gt9886,
    focal_ft3518
} TP_USED_INDEX;
TP_USED_INDEX tp_used_index  = TP_INDEX_NULL;

#define GET_TP_DEV_NAME(tp_type) ((tp_dev_names[tp_type].type == (tp_type))?tp_dev_names[tp_type].name:"UNMATCH")

#ifndef CONFIG_MTK_FB
void primary_display_esd_check_enable(int enable)
{
    return;
}
EXPORT_SYMBOL(primary_display_esd_check_enable);
#endif /*CONFIG_MTK_FB*/

bool __init tp_judge_ic_match(char *tp_ic_name)
{
	int prj_id = 0;
	prj_id = get_project();

	pr_err("[TP] tp_ic_name = %s \n", tp_ic_name);
	switch(prj_id) {

	case 20015:
	case 20016:
	case 20651:
	case 20652:
	case 20653:
	case 20654:
	case 20108:
	case 20109:
		pr_info("[TP] case 20015\n");
		if (strstr(tp_ic_name, "novatek,nf_nt36525b") && strstr(saved_command_line, "oppo20625_nt35625b_boe_hlt_b3_vdo_lcm_drv")) {
			pr_info("[TP] touch judge ic = novatek,nf_nt36525b,INX\n");
			return true;
		}
		if (strstr(tp_ic_name, "ilitek,ili9882n") && strstr(saved_command_line, "oppo20015_ili9882n_truly_hd_vdo_lcm_drv")) {
			pr_info("[TP] touch judge ic = ilitek,ili9882n\n");
			return true;
		}
		if (strstr(tp_ic_name, "ilitek,ili9882n") && strstr(saved_command_line, "oppo20015_ili9882n_boe_hd_vdo_lcm_drv")) {
			pr_info("[TP] touch judge ic = ilitek,ili9882n,HLT\n");
			return true;
		}
		if (strstr(tp_ic_name, "ilitek,ili9882n") && strstr(saved_command_line, "oppo20015_ili9882n_innolux_hd_vdo_lcm_drv")) {
			pr_info("[TP] touch judge ic = ilitek,ili9882n,INX\n");
			return true;
		}
		break;
	case 20608:
	case 20609:
	case 0x2060A:
	case 0x2060B:
	case 0x206F0:
	case 0x206FF:
	case 20796:
	case 20795:
	case 0x2070B:
	case 0x2070C:
	case 0x2070E:
		pr_info("[TP] case 20609\n");
		if (strstr(tp_ic_name, "novatek,nf_nt36672c") && strstr(saved_command_line, "oppo20609_nt36672c_tm_cphy_dsi_vdo_lcm_drv")) {
			pr_info("[TP] touch judge ic = novatek,nf_nt36672c,TIANMA\n");
			return true;
		}
		if (strstr(tp_ic_name, "ilitek,ili9882n") && strstr(saved_command_line, "oppo20609_ili7807s_tm_cphy_dsi_vdo_lcm_drv")) {
			pr_info("[TP] touch judge ic = ilitek,ili9882n(ili7807s),TIANMA\n");
			return true;
		}
		break;
	default:
		pr_err("Invalid project\n");
		break;
	}
	pr_err("Lcd module not found\n");
	return false;

}

bool  tp_judge_ic_match_commandline(struct panel_info *panel_data)
{
    int prj_id = 0;
    int i = 0;
    bool ic_matched = false;
    prj_id = get_project();
    pr_err("[TP] get_project() = %d \n", prj_id);
    pr_err("[TP] saved_command_line = %s \n", saved_command_line);
    for(i = 0; i < panel_data->project_num; i++){
        if(prj_id == panel_data->platform_support_project[i]) {
            g_tp_prj_id = panel_data->platform_support_project_dir[i];
            if(strstr(saved_command_line, panel_data->platform_support_commandline[i])
                || strstr("default_commandline", panel_data->platform_support_commandline[i])){
                pr_err("[TP] Driver match the project\n");
                ic_matched = true;
            }
        }
    }

    if (!ic_matched) {
        pr_err("[TP] Driver does not match the project\n");
        pr_err("Lcd module not found\n");
        return false;
    }

	switch(prj_id) {
	case 19131:
	case 19132:
	case 19133:
	case 19420:
	case 19421:
#ifdef CONFIG_MACH_MT6873
        pr_info("[TP] case 19131\n");
        is_tp_type_got_in_match = true;
        if (strstr(saved_command_line, "nt36672c_fhdp_dsi_vdo_auo_cphy_90hz_tianma")) {
            pr_err("[TP] touch ic = nt36672c_tianma \n");
            tp_used_index = nt36672c;
            g_tp_dev_vendor = TP_TIANMA;
        }
        if (strstr(saved_command_line, "nt36672c_fhdp_dsi_vdo_auo_cphy_120hz_tianma")) {
            pr_err("[TP] touch ic = nt36672c_tianma \n");
            tp_used_index = nt36672c;
            g_tp_dev_vendor = TP_TIANMA;
        }
        if (strstr(saved_command_line, "nt36672c_fhdp_dsi_vdo_auo_cphy_90hz_jdi")) {
            pr_err("[TP] touch ic = nt36672c_jdi \n");
            tp_used_index = nt36672c;
            g_tp_dev_vendor = TP_JDI;
        }
        if (strstr(saved_command_line, "hx83112f_fhdp_dsi_vdo_auo_cphy_90hz_jdi")) {
            pr_err("[TP] touch ic = hx83112f_jdi \n");
            tp_used_index = himax_83112f;
            g_tp_dev_vendor = TP_JDI;
        }
#endif
        break;

	case 20001:
	case 20002:
	case 20003:
	case 20200:
        pr_info("[TP] case 20001\n");
        is_tp_type_got_in_match = true;
        if (strstr(saved_command_line, "nt36672c")) {
            pr_err("[TP] touch ic = nt36672c_jdi \n");
            tp_used_index = nt36672c;
            g_tp_dev_vendor = TP_JDI;
        }
        if (strstr(saved_command_line, "hx83112f")) {
            pr_err("[TP] touch ic = hx83112f_tianma \n");
            tp_used_index = himax_83112f;
            g_tp_dev_vendor = TP_TIANMA;
        }
        break;
	
	case 20015:
	case 20016:
	case 20651:
	case 20652:
	case 20653:
	case 20654:
	case 20108:
	case 20109:
		pr_info("[TP] case 20015\n");
		is_tp_type_got_in_match = true;

		if (strstr(saved_command_line, "oppo20625_nt35625b_boe_hlt_b3_vdo_lcm_drv")) {
			pr_info("[TP] touch ic = novatek,nf_nt36525b\n");
			g_tp_dev_vendor = TP_INX;
			tp_used_index = nt36525b_boe;
		}
		if (strstr(saved_command_line, "oppo20015_ili9882n_truly_hd_vdo_lcm_drv"))
		{
			pr_info("[TP] touch ic = tchip,ilitek\n");
			g_tp_dev_vendor = TP_CDOT;
			tp_used_index = ili9882n_cdot;
		}
		if (strstr(saved_command_line, "oppo20015_ili9882n_boe_hd_vdo_lcm_drv"))
		{
			pr_info("[TP] touch ic = tchip,ilitek,HLT\n");
			g_tp_dev_vendor = TP_HLT;
			tp_used_index = ili9882n_hlt;
		}
		if (strstr(saved_command_line, "oppo20015_ili9882n_innolux_hd_vdo_lcm_drv"))
		{
			pr_info("[TP] touch ic = tchip,ilitek,INX\n");
			g_tp_dev_vendor = TP_INX;
			tp_used_index = ili9882n_inx;
		}
		break;
	case 20608:
	case 20609:
	case 0x2060A:
	case 0x2060B:
	case 0x206F0:
	case 0x206FF:
	case 20796:
	case 20795:
	case 0x2070B:
	case 0x2070C:
	case 0x2070E:
		pr_info("[TP] case 20609\n");
		is_tp_type_got_in_match = true;

		if (strstr(saved_command_line, "oppo20609_nt36672c_tm_cphy_dsi_vdo_lcm_drv")) {
			pr_info("[TP] touch ic = novatek,nf_nt6672c\n");
			g_tp_dev_vendor = TP_TIANMA;
			tp_used_index = nt36672c;
		}
		if (strstr(saved_command_line, "oppo20609_ili7807s_tm_cphy_dsi_vdo_lcm_drv")) {
			pr_info("[TP] touch ic = novatek,ili9882n(ili7807s)\n");
			g_tp_dev_vendor = TP_TIANMA;
			tp_used_index = ili9882n_inx;
		}
		break;
	default:
        pr_info("other project, no need process here!\n");
        break;

    }

    return true;
}

int tp_util_get_vendor(struct hw_resource *hw_res, struct panel_info *panel_data)
{
    char *vendor;
    int prj_id = 0;
    prj_id = get_project();
    g_hw_res = hw_res;

    panel_data->test_limit_name = kzalloc(MAX_LIMIT_DATA_LENGTH, GFP_KERNEL);
    if (panel_data->test_limit_name == NULL) {
        pr_err("[TP]panel_data.test_limit_name kzalloc error\n");
    }

    panel_data->extra = kzalloc(MAX_LIMIT_DATA_LENGTH, GFP_KERNEL);
    if (panel_data->extra == NULL) {
        pr_err("[TP]panel_data.extra kzalloc error\n");
    }

    if (is_tp_type_got_in_match) {
        panel_data->tp_type = g_tp_dev_vendor;
    }
    if (panel_data->tp_type == TP_UNKNOWN) {
        pr_err("[TP]%s type is unknown\n", __func__);
        return 0;
    }
    /*
    if (prj_id == 19165 || prj_id == 19166) {
        memcpy(panel_data->manufacture_info.version, "0xBD3100000", 11);
    }
    if (prj_id == 20075 || prj_id == 20076) {
        memcpy(panel_data->manufacture_info.version, "0xRA5230000", 11);
    }
    */
    if (prj_id == 20131 || prj_id == 20133 || prj_id == 20255 || prj_id == 20257) {
        memcpy(panel_data->manufacture_info.version, "0xbd3520000", 11);
    }
    vendor = GET_TP_DEV_NAME(panel_data->tp_type);

    strcpy(panel_data->manufacture_info.manufacture, vendor);

	switch(prj_id) {
	case 19131:
	case 19132:
	case 19133:
	case 19420:
	case 19421:
#ifdef CONFIG_MACH_MT6873
        pr_info("[TP] enter case OPPO_19131\n");
        if (tp_used_index == nt36672c) {
            snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
                     "tp/19131/FW_%s_%s.img",
                     "NF_NT36672C", vendor);

            if (panel_data->test_limit_name) {
                snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
                         "tp/19131/LIMIT_%s_%s.img",
                         "NF_NT36672C", vendor);
            }

            if (panel_data->extra) {
                snprintf(panel_data->extra, MAX_LIMIT_DATA_LENGTH,
                         "tp/19131/BOOT_FW_%s_%s.ihex",
                         "NF_NT36672C", vendor);
            }
            panel_data->manufacture_info.fw_path = panel_data->fw_name;
            if ((tp_used_index == nt36672c) && (g_tp_dev_vendor == TP_JDI)) {
                pr_info("[TP]: firmware_headfile = FW_19131_NF_NT36672C_JDI_fae_jdi\n");
                memcpy(panel_data->manufacture_info.version, "0xDD300JN200", 12);
                //panel_data->firmware_headfile.firmware_data = FW_19131_NF_NT36672C_JDI;
                //panel_data->firmware_headfile.firmware_size = sizeof(FW_19131_NF_NT36672C_JDI);
                panel_data->firmware_headfile.firmware_data = FW_19131_NF_NT36672C_JDI_fae_jdi;
                panel_data->firmware_headfile.firmware_size = sizeof(FW_19131_NF_NT36672C_JDI_fae_jdi);
            }
            if ((tp_used_index == nt36672c) && (g_tp_dev_vendor == TP_TIANMA)) {
                pr_info("[TP]: firmware_headfile = FW_19131_NF_NT36672C_TIANMA_fae_tianma\n");
                memcpy(panel_data->manufacture_info.version, "0xDD300TN000", 12);
                //panel_data->firmware_headfile.firmware_data = FW_19131_NF_NT36672C_TIANMA_realme;
                //panel_data->firmware_headfile.firmware_size = sizeof(FW_19131_NF_NT36672C_TIANMA_realme);
                panel_data->firmware_headfile.firmware_data = FW_19131_NF_NT36672C_TIANMA_fae_tianma;
                panel_data->firmware_headfile.firmware_size = sizeof(FW_19131_NF_NT36672C_TIANMA_fae_tianma);
            }
        }

        if (tp_used_index == himax_83112f) {
            snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
                     "tp/19131/FW_%s_%s.img",
                     "NF_HX83112F", vendor);

            if (panel_data->test_limit_name) {
                snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
                         "tp/19131/LIMIT_%s_%s.img",
                         "NF_HX83112F", vendor);
            }

            if (panel_data->extra) {
                snprintf(panel_data->extra, MAX_LIMIT_DATA_LENGTH,
                         "tp/19131/BOOT_FW_%s_%s.ihex",
                         "NF_HX83112F", vendor);
            }
            panel_data->manufacture_info.fw_path = panel_data->fw_name;
            if (tp_used_index == himax_83112f) {
                pr_info("[TP]: firmware_headfile = FW_19131_NF_HX83112F_JDI\n");
                memcpy(panel_data->manufacture_info.version, "0xDD300JH000", 12);
                panel_data->firmware_headfile.firmware_data = FW_19131_NF_HX83112F_JDI;
                panel_data->firmware_headfile.firmware_size = sizeof(FW_19131_NF_HX83112F_JDI);
            }
        }
#endif
        break;
	case 20051:
        pr_info("[TP] enter case OPPO_20051\n");
        if (tp_used_index == nt36672c) {
            snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
                     "tp/20051/FW_%s_%s.img",
                     "NF_NT36672C", "JDI");

            if (panel_data->test_limit_name) {
                snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
                         "tp/20051/LIMIT_%s_%s.img",
                         "NF_NT36672C", "JDI");
            }
            if (panel_data->extra) {
                snprintf(panel_data->extra, MAX_LIMIT_DATA_LENGTH,
                         "tp/20051/BOOT_FW_%s_%s.ihex",
                         "NF_NT36672C", "JDI");
            }
            panel_data->manufacture_info.fw_path = panel_data->fw_name;
            if ((tp_used_index == nt36672c) && (g_tp_dev_vendor == TP_JDI)) {
                pr_info("[TP]: firmware_headfile = FW_20051_NF_NT36672C_JDI_fae_jdi\n");
                memcpy(panel_data->manufacture_info.version, "0xBD358JN200", 12);
                panel_data->firmware_headfile.firmware_data = FW_20051_NF_NT36672C_JDI_fae_jdi;
                panel_data->firmware_headfile.firmware_size = sizeof(FW_20051_NF_NT36672C_JDI_fae_jdi);
            }
        }
        break;
	case 20001:
	case 20002:
	case 20003:
	case 20200:
        pr_info("[TP] enter case OPPO_20001\n");
        if (tp_used_index == nt36672c) {
            snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
                     "tp/20001/FW_%s_%s.img",
                     "NF_NT36672C", vendor);

            if (panel_data->test_limit_name) {
                snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
                         "tp/20001/LIMIT_%s_%s.img",
                         "NF_NT36672C", vendor);
            }
            if (panel_data->extra) {
                snprintf(panel_data->extra, MAX_LIMIT_DATA_LENGTH,
                         "tp/20001/BOOT_FW_%s_%s.ihex",
                         "NF_NT36672C", vendor);
            }
            panel_data->manufacture_info.fw_path = panel_data->fw_name;
            if (tp_used_index == nt36672c) {
                pr_info("[TP]: firmware_headfile = FW_20001_NF_NT36672C_JDI_fae_jdi\n");
                memcpy(panel_data->manufacture_info.version, "0xFA219DN", 9);
                panel_data->firmware_headfile.firmware_data = FW_20001_NF_NT36672C_JDI;
                panel_data->firmware_headfile.firmware_size = sizeof(FW_20001_NF_NT36672C_JDI);
            }
        }

        if (tp_used_index == himax_83112f) {
            snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
                     "tp/20001/FW_%s_%s.img",
                     "NF_HX83112F", vendor);

            if (panel_data->test_limit_name) {
                snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
                         "tp/20001/LIMIT_%s_%s.img",
                         "NF_HX83112F", vendor);
            }

            if (panel_data->extra) {
                snprintf(panel_data->extra, MAX_LIMIT_DATA_LENGTH,
                         "tp/20001/BOOT_FW_%s_%s.ihex",
                         "NF_HX83112F", vendor);
            }
            panel_data->manufacture_info.fw_path = panel_data->fw_name;
            if (tp_used_index == himax_83112f) {
                pr_info("[TP]: firmware_headfile = FW_20001_NF_HX83112F_TIANMA\n");
                memcpy(panel_data->manufacture_info.version, "0xFA219TH", 9);
                panel_data->firmware_headfile.firmware_data = FW_20001_NF_HX83112F_TIANMA;
                panel_data->firmware_headfile.firmware_size = sizeof(FW_20001_NF_HX83112F_TIANMA);
            }
        }
        break;
	case 20015:
	case 20016:
	case 20651:
	case 20652:
	case 20653:
	case 20654:
	case 20108:
	case 20109:
		pr_info("[TP] enter case 20015\n");
		if (tp_used_index == nt36525b_boe) {
			snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
					 "tp/20015/FW_%s_%s.bin",
					 "NF_NT36525B", vendor);
			if (panel_data->test_limit_name) {
				snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
						 "tp/20015/LIMIT_%s_%s.img",
						 "NF_NT36525B", vendor);
			}
			panel_data->manufacture_info.fw_path = panel_data->fw_name;
			memcpy(panel_data->manufacture_info.version, "0xAA006IN", 9);
			panel_data->firmware_headfile.firmware_data = FW_20015_NT36525B_INX;
			panel_data->firmware_headfile.firmware_size = sizeof(FW_20015_NT36525B_INX);
		}
		if (tp_used_index == ili9882n_cdot) {
			snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
					 "tp/20015/FW_%s_%s.bin",
					 "NF_ILI9882N", vendor);
			if (panel_data->test_limit_name) {
				snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
						 "tp/20015/LIMIT_%s_%s.ini",
						 "NF_ILI9882N", vendor);
			}
			panel_data->manufacture_info.fw_path = panel_data->fw_name;
			memcpy(panel_data->manufacture_info.version, "ILI9882N_", 9);
			panel_data->firmware_headfile.firmware_data = FW_20015_ILI9882N_CDOT;
			panel_data->firmware_headfile.firmware_size = sizeof(FW_20015_ILI9882N_CDOT);
		}
		if (tp_used_index == ili9882n_hlt) {
			snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
					 "tp/20015/FW_%s_%s.bin",
					 "NF_ILI9882N", vendor);
			if (panel_data->test_limit_name) {
				snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
						 "tp/20015/LIMIT_%s_%s.ini",
						 "NF_ILI9882N", vendor);
			}
			panel_data->manufacture_info.fw_path = panel_data->fw_name;
			memcpy(panel_data->manufacture_info.version, "0xAA006HI", 9);
			panel_data->firmware_headfile.firmware_data = FW_20015_ILI9882N_HLT;
			panel_data->firmware_headfile.firmware_size = sizeof(FW_20015_ILI9882N_HLT);
		}
		if (tp_used_index == ili9882n_inx) {
			snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
					 "tp/20015/FW_%s_%s.bin",
					 "NF_ILI9882N", vendor);
			if (panel_data->test_limit_name) {
				snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
						 "tp/20015/LIMIT_%s_%s.ini",
						 "NF_ILI9882N", vendor);
			}
			panel_data->manufacture_info.fw_path = panel_data->fw_name;
			memcpy(panel_data->manufacture_info.version, "0xAA006II", 9);
			panel_data->firmware_headfile.firmware_data = FW_20015_ILI9882N_INX;
			panel_data->firmware_headfile.firmware_size = sizeof(FW_20015_ILI9882N_INX);
		}
		break;
	case 20608:
	case 20609:
	case 0x2060A:
	case 0x2060B:
	case 0x206F0:
	case 0x206FF:
	case 20796:
	case 20795:
	case 0x2070B:
	case 0x2070C:
	case 0x2070E:
		pr_info("[TP] enter case 20609\n");
		if (tp_used_index == nt36672c) {
			snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
					 "tp/20609/FW_%s_%s.bin",
					 "NF_NT36672C", vendor);
			if (panel_data->test_limit_name) {
				snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
						 "tp/20609/LIMIT_%s_%s.img",
						 "NF_NT36672C", vendor);
			}
			panel_data->manufacture_info.fw_path = panel_data->fw_name;
			memcpy(panel_data->manufacture_info.version, "RA627_TM_NT_", 12);
			panel_data->firmware_headfile.firmware_data = FW_20609_NT36672C_TIANMA;
			panel_data->firmware_headfile.firmware_size = sizeof(FW_20609_NT36672C_TIANMA);
		}
		if (tp_used_index == ili9882n_inx) {
			snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
					 "tp/20609/FW_%s_%s.bin",
					 "NF_ILI7807S", vendor);
			if (panel_data->test_limit_name) {
				snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
						 "tp/20609/LIMIT_%s_%s.img",
						 "NF_ILI7807S", vendor);
			}
			panel_data->manufacture_info.fw_path = panel_data->fw_name;
			memcpy(panel_data->manufacture_info.version, "RA627_TM_ILI_", 13);
			panel_data->firmware_headfile.firmware_data = FW_20609_ILI7807S_TIANMA;
			panel_data->firmware_headfile.firmware_size = sizeof(FW_20609_ILI7807S_TIANMA);
		}
		break;
	default:
        if(!strcmp(vendor, "SAMSUNG")) {
            snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
                "tp/%d/FW_%s_%s.bin",
                prj_id, panel_data->chip_name, vendor);
        } else {
            snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
                 "tp/%d/FW_%s_%s.img",
                 prj_id, panel_data->chip_name, vendor);
        }
        if (panel_data->test_limit_name) {
            snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
                     "tp/%d/LIMIT_%s_%s.img",
                     prj_id, panel_data->chip_name, vendor);
        }

        if (panel_data->extra) {
            snprintf(panel_data->extra, MAX_LIMIT_DATA_LENGTH,
                     "tp/%d/BOOT_FW_%s_%s.ihex",
                     prj_id, panel_data->chip_name, vendor);
        }
        panel_data->manufacture_info.fw_path = panel_data->fw_name;
        break;
    }

    pr_info("[TP]vendor:%s fw:%s limit:%s\n",
        vendor,
        panel_data->fw_name,
        panel_data->test_limit_name == NULL ? "NO Limit" : panel_data->test_limit_name);

    return 0;
}
void switch_spi7cs_state(bool normal)
{
	if (normal) {
		if (!IS_ERR_OR_NULL(g_hw_res->pin_set_high)) {
			pr_info("%s: going to set spi7 cs to spi mode .\n", __func__);
			pinctrl_select_state(g_hw_res->pinctrl, g_hw_res->pin_set_high);
		} else {
			pr_info("%s: cannot to set spi7 cs to spi mode .\n", __func__);
		}
	} else {
		if (!IS_ERR_OR_NULL(g_hw_res->pin_set_low)) {
			pr_info("%s: going to set spi7 cs to pulldown .\n", __func__);
			pinctrl_select_state(g_hw_res->pinctrl, g_hw_res->pin_set_low);
		} else {
			pr_info("%s: cannot to set spi7 cs to pulldown .\n", __func__);
		}
	}
}
EXPORT_SYMBOL(switch_spi7cs_state);

