/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "imgsensor_eeprom.h"
#include <soc/oppo/device_info.h>
#include <soc/oplus/system/oppo_project.h>
#define DUMP_EEPROM 0

/*GC8054: 0xA2*/
/*static IMGSENSOR_EEPROM_MODULE_INFO gEpInfoObj = {
    .i4SlaveAddr = {0xA0, 0xA8, 0xA2, 0xA8},
};*/

kal_uint16 Eeprom_1ByteDataRead(kal_uint16 addr, kal_uint16 slaveaddr)
{
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(pusendcmd , 2, (u8*)&get_byte, 1, slaveaddr);
    return get_byte;
}

#if 0
static enum IMGSENSOR_RETURN Eeprom_QcomHeaderCheck(kal_uint8 *header)
{
    unsigned int   sum = 0, i = 0;
    if (header == NULL) {
        pr_debug("Eeprom_QcomHeaderCheck is NULL");
        return IMGSENSOR_RETURN_ERROR;
    }

    for (i = 0; i < OPPO_STEREO_LENGTH_QCOMHEADER - 1; i++)
    {
        sum += header[i];
    }

    sum &= 0xFF;
    sum ^= 0xFF;

    if (sum == header[OPPO_STEREO_LENGTH_QCOMHEADER - 1])
        return IMGSENSOR_RETURN_SUCCESS;
    else
        return IMGSENSOR_RETURN_ERROR;
}
#endif

void Eeprom_DataRead(kal_uint8 *uData, kal_uint16 dataAddr,
                            kal_uint16 dataLens, kal_uint16 slaveAddr)
{
    int dataCnt = 0;
    for (dataCnt = 0; dataCnt < dataLens; dataCnt++) {
        uData[dataCnt] = (u8)Eeprom_1ByteDataRead(dataAddr+dataCnt, slaveAddr);
    }
}

static enum IMGSENSOR_RETURN Eeprom_TableWrite(kal_uint16 addr, kal_uint8 *para,
                                    kal_uint32 len, kal_uint16 i4SlaveAddr)
{
    enum IMGSENSOR_RETURN ret = IMGSENSOR_RETURN_SUCCESS;
    char pusendcmd[WRITE_DATA_MAX_LENGTH+2];
    pusendcmd[0] = (char)(addr >> 8);
    pusendcmd[1] = (char)(addr & 0xFF);

    memcpy(&pusendcmd[2], para, len);

    ret = iBurstWriteReg((kal_uint8 *)pusendcmd , (len + 2), i4SlaveAddr);

    return ret;
}

static enum IMGSENSOR_RETURN Eeprom_WriteProtectEnable(kal_uint16 enable, kal_uint16 i4SlaveAddr)
{
    enum IMGSENSOR_RETURN ret = IMGSENSOR_RETURN_SUCCESS;
    char pusendcmd[3];
    pusendcmd[0] = 0x80;
    pusendcmd[1] = 0x00;
    if (enable)
        pusendcmd[2] = 0xE0;
    else
        pusendcmd[2] = 0x00;

    ret = iBurstWriteReg((kal_uint8 *)pusendcmd , 3, i4SlaveAddr);

    return ret;
}

static enum IMGSENSOR_RETURN Eeprom_WriteInfomatch(
                ACDK_SENSOR_ENGMODE_STEREO_STRUCT *pStereoData)
{
    kal_uint16 data_base, data_length;
    kal_uint32 uSensorId;
    if(pStereoData == NULL) {
        pr_debug("SET_SENSOR_OTP pStereoData is NULL!");
        return IMGSENSOR_RETURN_ERROR;
    }

    uSensorId   = pStereoData->uSensorId;
    data_base   = pStereoData->baseAddr;
    data_length = pStereoData->dataLength;

    switch (uSensorId) {
        #if defined(OV64B_MIPI_RAW)
        case OV64B_SENSOR_ID:
        {
            if (data_length == CALI_DATA_MASTER_LENGTH
                && data_base == OV64B_STEREO_START_ADDR) {
                pr_debug("ov64b DA:0x%x DL:%d", data_base, data_length);
            } else {
                pr_debug("ov64b Invalid DA:0x%x DL:%d", data_base, data_length);
                return IMGSENSOR_RETURN_ERROR;
            }
            break;
        }
        #endif
        #if defined(IMX319_MIPI_RAW)
        case IMX319_SENSOR_ID:
        {
            if (data_length == CALI_DATA_SLAVE_LENGTH
                && data_base == IMX319_STEREO_START_ADDR) {
                pr_debug("imx319 DA:0x%x DL:%d", data_base, data_length);
            } else {
                pr_debug("imx319 Invalid DA:0x%x DL:%d", data_base, data_length);
                return IMGSENSOR_RETURN_ERROR;
            }
            break;
        }
        #endif
        default:
            pr_debug("Invalid SensorID:0x%x", uSensorId);
            return IMGSENSOR_RETURN_ERROR;
    }

    return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN Eeprom_WriteData(kal_uint16 data_base, kal_uint16 data_length,
                                   kal_uint8 *pData,  kal_uint16 i4SlaveAddr)
{
    enum IMGSENSOR_RETURN ret = IMGSENSOR_RETURN_SUCCESS;
    kal_uint16 max_Lens = WRITE_DATA_MAX_LENGTH;
    kal_uint32 idx, idy, i = 0;
    if (pData == NULL || data_length < WRITE_DATA_MAX_LENGTH) {
        pr_err("Eeprom_WriteData pData is NULL or invalid DL:%d",data_length );
        return IMGSENSOR_RETURN_ERROR;
    }

    idx = data_length/max_Lens;
    idy = data_length%max_Lens;
    pr_debug("Eeprom_WriteData: 0x%x %d i4SlaveAddr:0x%x\n",
                data_base,
                data_length,
                i4SlaveAddr);
    /* close write protect */
    Eeprom_WriteProtectEnable(0, i4SlaveAddr);
    msleep(6);
    for (i = 0; i < idx; i++ ) {
        ret = Eeprom_TableWrite((data_base+max_Lens*i),
                &pData[max_Lens*i], max_Lens, i4SlaveAddr);
        if (ret != IMGSENSOR_RETURN_SUCCESS) {
            pr_err("write_eeprom error: i= %d\n", i);
            /* open write protect */
            Eeprom_WriteProtectEnable(1, i4SlaveAddr);
            msleep(6);
            return IMGSENSOR_RETURN_ERROR;
        }
        msleep(6);
    }
    ret = Eeprom_TableWrite((data_base+max_Lens*idx),
            &pData[max_Lens*idx], idy, i4SlaveAddr);
    if (ret != IMGSENSOR_RETURN_SUCCESS) {
        pr_err("Eeprom_TableWrite error: idx= %d idy= %d\n", idx, idy);
        /* open write protect */
        Eeprom_WriteProtectEnable(1, i4SlaveAddr);
        msleep(6);
        return IMGSENSOR_RETURN_ERROR;
    }
    msleep(6);
    /* open write protect */
    Eeprom_WriteProtectEnable(1, i4SlaveAddr);
    msleep(6);

    return ret;
}

enum IMGSENSOR_RETURN Eeprom_SensorInfoValid(
            enum IMGSENSOR_SENSOR_IDX sensor_idx,
            kal_uint32 sensorID)
{

    struct CAMERA_DEVICE_INFO *pCamDeviceObj = &gImgEepromInfo_20131;

    if(is_project(20075) || is_project(20076))
        pCamDeviceObj = &gImgEepromInfo_20075;

    if (sensor_idx > pCamDeviceObj->i4SensorNum - 1) {
        pr_info("[%s] sensor_idx:%d > i4SensorNum: %d", __func__, sensor_idx, pCamDeviceObj->i4SensorNum);
        return IMGSENSOR_RETURN_ERROR;
    }

    if (sensorID != pCamDeviceObj->pCamModuleInfo[sensor_idx].i4Sensorid) {
        pr_info("[%s] sensorID:%d mismatch i4Sensorid: %d",
                __func__, sensorID, pCamDeviceObj->pCamModuleInfo[sensor_idx].i4Sensorid);
        return IMGSENSOR_RETURN_ERROR;
    }

    return IMGSENSOR_RETURN_SUCCESS;
}

enum IMGSENSOR_RETURN Eeprom_CallWriteService(ACDK_SENSOR_ENGMODE_STEREO_STRUCT * pStereoData)
{
    enum IMGSENSOR_RETURN  ret = IMGSENSOR_RETURN_SUCCESS;
    kal_uint16 data_base = 0, data_length = 0;
    kal_uint8 *pData;
    kal_uint16 i4SlaveAddr = 0xFF;
    kal_uint32 uSensorId = 0x0, uDeviceId = DUAL_CAMERA_MAIN_SENSOR;
    enum IMGSENSOR_SENSOR_IDX sensor_idx = IMGSENSOR_SENSOR_IDX_MAIN;
    struct CAMERA_DEVICE_INFO *pCamDeviceObj = &gImgEepromInfo_20131;

    #if DUMP_EEPROM
    kal_uint8 uData[CALI_DATA_MASTER_LENGTH];
    int i = 0;
    #endif

    if (is_project(20075) || is_project(20076))
        pCamDeviceObj = &gImgEepromInfo_20075;

    if (pStereoData == NULL) {
        pr_err("Eeprom_CallWriteService pStereoData is NULL");
        return IMGSENSOR_RETURN_ERROR;
    }
    pr_debug("SET_SENSOR_OTP: 0x%x %d 0x%x %d\n",
                pStereoData->uSensorId,
                pStereoData->uDeviceId,
                pStereoData->baseAddr,
                pStereoData->dataLength);

    data_base   = pStereoData->baseAddr;
    data_length = pStereoData->dataLength;
    uSensorId   = pStereoData->uSensorId;
    uDeviceId   = pStereoData->uDeviceId;
    pData       = pStereoData->uData;

    /*match iic slave addr*/
    if (uDeviceId == DUAL_CAMERA_MAIN_SENSOR) {
        sensor_idx = IMGSENSOR_SENSOR_IDX_MAIN;
    } else if (uDeviceId == DUAL_CAMERA_SUB_SENSOR) {
        sensor_idx = IMGSENSOR_SENSOR_IDX_SUB;
    } else if (uDeviceId == DUAL_CAMERA_MAIN_2_SENSOR) {
        sensor_idx = IMGSENSOR_SENSOR_IDX_MAIN2;
    } else if (uDeviceId == DUAL_CAMERA_SUB_2_SENSOR) {
        sensor_idx = IMGSENSOR_SENSOR_IDX_SUB2;
    } else if (uDeviceId == DUAL_CAMERA_MAIN_3_SENSOR) {
        sensor_idx = IMGSENSOR_SENSOR_IDX_MAIN3;
    } else if (uDeviceId == DUAL_CAMERA_MAIN_4_SENSOR) {
        sensor_idx = IMGSENSOR_SENSOR_IDX_MAIN4;
    }

    /*eeprom info check*/
    ret = Eeprom_WriteInfomatch(pStereoData);
    if (ret != IMGSENSOR_RETURN_SUCCESS) {
        pr_err("Eeprom_WriteInfomatch failed: %d", ret);
        return IMGSENSOR_RETURN_ERROR;
    }

    /*Check sensorid & senidx*/
    if (Eeprom_SensorInfoValid(sensor_idx, uSensorId) != IMGSENSOR_RETURN_SUCCESS) {
        pr_info("[%s]sensor_idx:%d, uSensorId:0x%x Invalid", __func__, sensor_idx, uSensorId);
        return IMGSENSOR_RETURN_ERROR;
    }
    /*Match EEPROM IIC-ADDR EEPROM OFFSET*/
    i4SlaveAddr = pCamDeviceObj->pCamModuleInfo[sensor_idx].i4SlaveAddr;
    if (i4SlaveAddr == 0xFF || i4SlaveAddr == 0) {
        pr_info("[%s] uSensorId:%d is not support Eeprom Hardware", __func__, uSensorId);
        return IMGSENSOR_RETURN_ERROR;
    }

    /*write eeprom action*/
    ret = Eeprom_WriteData(data_base, data_length, pData, i4SlaveAddr);
    if (ret != IMGSENSOR_RETURN_SUCCESS) {
        pr_err("Eeprom_WriteData failed: %d", ret);
        return IMGSENSOR_RETURN_ERROR;
    }
    pr_debug("Eeprom_CallWriteService return SCUESS:%d\n", ret);
    #if DUMP_EEPROM
    Eeprom_DataRead(uData, data_base, data_length, i4SlaveAddr);
    ret = strncmp(uData, pData, data_length);
    pr_info("cmp uData pData, ret = %d", ret);
    if (ret) {
        for (i - 0; i < data_length; ++i) {
            pr_debug("pData addr: 0x%x, data: 0x%x", i, pData[i]);
            pr_debug("uData addr: 0x%x, data: 0x%x", i, uData[i]);
        }
    }
    #endif
    return ret;
}

void Eeprom_CamInfoDataRead(enum IMGSENSOR_SENSOR_IDX sensor_idx, kal_uint16 slaveAddr)
{

    kal_uint16 dataLens = 2, dataAddr = 0xFFFF, dataCnt = 0;
    kal_uint8 module_id = 0xFF;
    struct CAMERA_DEVICE_INFO *pCamDeviceObj = &gImgEepromInfo_20131;
    if (is_project(20075) || is_project(20076))
    pCamDeviceObj = &gImgEepromInfo_20075;

    /*Read Module Info Low-2bytes*/
    dataAddr = pCamDeviceObj->pCamModuleInfo[sensor_idx].i4CamInfoAddr[0];
    dataLens = 2;
    dataCnt = OPPO_CAMCOMDATA_LENS*sensor_idx;
    Eeprom_DataRead(&pCamDeviceObj->CamNormdata[dataCnt], dataAddr, dataLens, slaveAddr);
    module_id = (kal_uint8)Eeprom_1ByteDataRead(dataAddr, slaveAddr);
    /*Register Module Info*/
    Oplusimgsensor_Registdeviceinfo(pCamDeviceObj->pCamModuleInfo[sensor_idx].name,
                                    pCamDeviceObj->pCamModuleInfo[sensor_idx].version,
                                    module_id);
    /*Read Module Info High-6bytes*/
    dataAddr = pCamDeviceObj->pCamModuleInfo[sensor_idx].i4CamInfoAddr[1];
    dataLens = 6;
    dataCnt += 2;
    Eeprom_DataRead(&pCamDeviceObj->CamNormdata[dataCnt], dataAddr, dataLens, slaveAddr);
}

void Eeprom_CamSNDataRead(enum IMGSENSOR_SENSOR_IDX sensor_idx, kal_uint16 slaveAddr)
{

    kal_uint16 dataLens = OPPO_CAMERASN_LENS, dataAddr = 0xFFFF, dataCnt = 0;
    struct CAMERA_DEVICE_INFO *pCamDeviceObj = &gImgEepromInfo_20131;
    if (is_project(20075) || is_project(20076))
        pCamDeviceObj = &gImgEepromInfo_20075;

    /*Read Module Info Low-2bytes*/
    dataAddr = pCamDeviceObj->pCamModuleInfo[sensor_idx].i4CamSNAddr;
    dataCnt = OPPO_CAMCOMDATA_LENS*sensor_idx + OPPO_CAMMODULEINFO_LENS;
    Eeprom_DataRead(&pCamDeviceObj->CamNormdata[dataCnt], dataAddr, dataLens, slaveAddr);
}

void Eeprom_CamAFCodeDataRead(enum IMGSENSOR_SENSOR_IDX sensor_idx, kal_uint16 slaveAddr)
{

    kal_uint16 dataLens = OPPO_CAMAFCODE_LENS, dataAddr = 0xFFFF, dataCnt = 0;
    kal_uint16 i = 0;
    struct CAMERA_DEVICE_INFO *pCamDeviceObj = &gImgEepromInfo_20131;
    if (is_project(20075) || is_project(20076))
        pCamDeviceObj = &gImgEepromInfo_20075;

    /*Read AF MAC+50+100+Inf+StereoDac*/
    if (pCamDeviceObj->pCamModuleInfo[sensor_idx].i4AfSupport)
    for (i = 0; i < 4; i ++) {
        dataAddr = pCamDeviceObj->pCamModuleInfo[sensor_idx].i4AFCodeAddr[i];
        dataCnt = OPPO_CAMCOMDATA_LENS*sensor_idx + OPPO_CAMMODULEINFO_LENS + OPPO_CAMERASN_LENS;
        dataCnt += 2*i;
        Eeprom_DataRead(&pCamDeviceObj->CamNormdata[dataCnt], dataAddr, dataLens, slaveAddr);
    }

    /*Set stereoFlag*/
    if ((pCamDeviceObj->i4MWDataIdx == sensor_idx)
         || (pCamDeviceObj->i4MTDataIdx == sensor_idx)
         || (pCamDeviceObj->i4FrontDataIdx == sensor_idx)) {
        dataCnt = OPPO_CAMCOMDATA_LENS*sensor_idx + OPPO_CAMMODULEINFO_LENS + OPPO_CAMERASN_LENS;
        dataCnt += OPPO_CAMAFCODE_LENS - 1;
        pCamDeviceObj->CamNormdata[dataCnt] = (char)(sensor_idx);
    }
}

void Eeprom_StereoDataRead(enum IMGSENSOR_SENSOR_IDX sensor_idx, kal_uint16 slaveAddr)
{

    kal_uint16 dataLens = OPPO_CAMAFCODE_LENS, dataAddr = 0xFFFF, dataCnt = 0;
    struct CAMERA_DEVICE_INFO *pCamDeviceObj = &gImgEepromInfo_20131;
    if (is_project(20075) || is_project(20076))
        pCamDeviceObj = &gImgEepromInfo_20075;

    /*Read Single StereoParamsData and joint stereoData*/
    if (pCamDeviceObj->i4MWDataIdx == IMGSENSOR_SENSOR_IDX_MAIN2) {
        if (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN) {
            dataLens = CALI_DATA_MASTER_LENGTH;
            dataAddr = pCamDeviceObj->i4MWStereoAddr[0];
        } else if (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN2) {
            dataLens = CALI_DATA_SLAVE_LENGTH;
            dataAddr = pCamDeviceObj->i4MWStereoAddr[1];
            dataCnt = CALI_DATA_MASTER_LENGTH;
        }
        Eeprom_DataRead(&pCamDeviceObj->stereoMWdata[dataCnt], dataAddr, dataLens, slaveAddr);
    } else if (pCamDeviceObj->i4MWDataIdx == IMGSENSOR_SENSOR_IDX_SUB2) {
        if (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN) {
            dataLens = CALI_DATA_MASTER_LENGTH;
            dataAddr = pCamDeviceObj->i4MTStereoAddr[0];
        } else if (sensor_idx == IMGSENSOR_SENSOR_IDX_SUB2) {
            dataLens = CALI_DATA_SLAVE_LENGTH;
            dataAddr = pCamDeviceObj->i4MTStereoAddr[1];
            dataCnt = CALI_DATA_MASTER_LENGTH;
        }
        Eeprom_DataRead(&pCamDeviceObj->stereoMTdata[dataCnt], dataAddr, dataLens, slaveAddr);
    } else if (pCamDeviceObj->i4MWDataIdx == IMGSENSOR_SENSOR_IDX_MAIN4) {
        if (sensor_idx == IMGSENSOR_SENSOR_IDX_SUB) {
            dataLens = CALI_DATA_MASTER_LENGTH;
            dataAddr = pCamDeviceObj->i4FrontStereoAddr[0];
        } else if (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN4) {
            dataLens = CALI_DATA_SLAVE_LENGTH;
            dataAddr = pCamDeviceObj->i4FrontStereoAddr[1];
            dataCnt = CALI_DATA_MASTER_LENGTH;
        }
        Eeprom_DataRead(&pCamDeviceObj->stereoFrontdata[dataCnt], dataAddr, dataLens, slaveAddr);
    }
}

enum IMGSENSOR_RETURN Eeprom_DataInit(
            enum IMGSENSOR_SENSOR_IDX sensor_idx,
            kal_uint32 sensorID)
{

    kal_uint16 slaveAddr = 0xA0;
    kal_uint8 module_id = 0xFF;
    struct CAMERA_DEVICE_INFO *pCamDeviceObj = &gImgEepromInfo_20131;
    if (is_project(20075) || is_project(20076))
        pCamDeviceObj = &gImgEepromInfo_20075;

    if (Eeprom_SensorInfoValid(sensor_idx, sensorID) != IMGSENSOR_RETURN_SUCCESS) {
        pr_info("[%s]sensor_idx:%d, sensorID:0x%x Invalid", __func__, sensor_idx, sensorID);
        return IMGSENSOR_RETURN_ERROR;
    }
    /*Match EEPROM IIC-ADDR EEPROM OFFSET*/
    slaveAddr = pCamDeviceObj->pCamModuleInfo[sensor_idx].i4SlaveAddr;
    if (slaveAddr == 0xFF) {
        pr_info("[%s] sensorID:%d is not support Eeprom Hardware", __func__, sensorID);
        Oplusimgsensor_Registdeviceinfo(pCamDeviceObj->pCamModuleInfo[sensor_idx].name,
                                        pCamDeviceObj->pCamModuleInfo[sensor_idx].version,
                                        module_id);
        return IMGSENSOR_RETURN_ERROR;
    }
    pCamDeviceObj->i4CurSensorIdx = sensor_idx;
    pCamDeviceObj->i4CurSensorId = sensorID;
    /*Read ModuleInfo Data*/
    Eeprom_CamInfoDataRead(sensor_idx, slaveAddr);
    /*Read camera sn*/
    Eeprom_CamSNDataRead(sensor_idx, slaveAddr);
    /*Read SteroData*/
    Eeprom_StereoDataRead(sensor_idx, slaveAddr);
    /*Read af code*/
    Eeprom_CamAFCodeDataRead(sensor_idx, slaveAddr);

    return IMGSENSOR_RETURN_SUCCESS;
}

enum CUSTOM_CAMERA_ERROR_CODE_ENUM Eeprom_Control(
            enum IMGSENSOR_SENSOR_IDX sensor_idx,
            MSDK_SENSOR_FEATURE_ENUM feature_id,
            unsigned char *feature_para,
            kal_uint32 sensorID)
{
    kal_uint32 i4SensorID = 0;
    kal_uint32 dataCnt = 0;
    enum IMGSENSOR_SENSOR_IDX i4Sensor_idx = 0;
    struct CAMERA_DEVICE_INFO *pCamDeviceObj = &gImgEepromInfo_20131;

    if (is_project(20075) || is_project(20076))
        pCamDeviceObj = &gImgEepromInfo_20075;
    i4Sensor_idx = pCamDeviceObj->i4CurSensorIdx;
    i4SensorID = pCamDeviceObj->i4CurSensorId;

    switch (feature_id) {
        case SENSOR_FEATURE_GET_EEPROM_COMDATA:
        {
            if (Eeprom_SensorInfoValid(i4Sensor_idx, i4SensorID) != IMGSENSOR_RETURN_SUCCESS) {
                pr_info("[%s]_COMDATA i4Sensor_idx:%d, i4SensorID:0x%x Invalid",
                                                __func__, i4Sensor_idx, i4SensorID);
                return ERROR_MSDK_IS_ACTIVATED;
            }
            dataCnt = (kal_uint32)(CAMERA_EEPPROM_COMDATA_LENGTH*i4Sensor_idx);
            pr_debug("GET_EEPROM_COMDATA i4Sensor_idx %d, i4SensorID:0x%x, dataCnt:%d\n",
                        i4Sensor_idx, i4SensorID, dataCnt);
            memcpy(&feature_para[0], &pCamDeviceObj->CamNormdata[dataCnt], CAMERA_EEPPROM_COMDATA_LENGTH);
            break;
        }
        case SENSOR_FEATURE_GET_EEPROM_STEREODATA:
        {
            if (Eeprom_SensorInfoValid(i4Sensor_idx, i4SensorID) != IMGSENSOR_RETURN_SUCCESS) {
                pr_info("[%s]_STEREODATA i4Sensor_idx:%d, i4SensorID:0x%x Invalid",
                                                __func__, i4Sensor_idx, i4SensorID);
                return ERROR_MSDK_IS_ACTIVATED;
            }
            pr_debug("GET_EEPROM_STEREODATA i4Sensor_idx %d, stereoDataIdx:%d %d %d\n",
                        i4Sensor_idx,
                        pCamDeviceObj->i4MWDataIdx,
                        pCamDeviceObj->i4MTDataIdx,
                        pCamDeviceObj->i4FrontDataIdx);
            if (i4Sensor_idx == pCamDeviceObj->i4MWDataIdx) {
                memcpy(&feature_para[0], pCamDeviceObj->stereoMWdata, DUALCAM_CALI_DATA_LENGTH_TOTAL);
            } else if (i4Sensor_idx == pCamDeviceObj->i4MTDataIdx) {
                memcpy(&feature_para[0], pCamDeviceObj->stereoMTdata, DUALCAM_CALI_DATA_LENGTH_TOTAL);
            } else if (i4Sensor_idx == pCamDeviceObj->i4FrontDataIdx) {
                memcpy(&feature_para[0], pCamDeviceObj->stereoFrontdata, DUALCAM_CALI_DATA_LENGTH_TOTAL);
            }
            break;
        }
        case SENSOR_FEATURE_SET_SENSOR_OTP:
        {
            enum IMGSENSOR_RETURN ret = IMGSENSOR_RETURN_SUCCESS;
            pr_debug("SENSOR_FEATURE_SET_SENSOR_OTP\n");
            return ERROR_NONE;
            ret = Eeprom_CallWriteService((ACDK_SENSOR_ENGMODE_STEREO_STRUCT *)(feature_para));
            if (ret == IMGSENSOR_RETURN_SUCCESS)
                return ERROR_NONE;
            else
                return ERROR_MSDK_IS_ACTIVATED;
        }
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            pr_debug("SENSOR_FEATURE_CHECK_SENSOR_ID: %d 0x%x\n", sensor_idx, sensorID);
            Eeprom_DataInit(sensor_idx, sensorID);
        break;
        default:
            pr_err("Invalid feature_id: %d", feature_id);
        break;
    }

    return ERROR_NONE;
}
