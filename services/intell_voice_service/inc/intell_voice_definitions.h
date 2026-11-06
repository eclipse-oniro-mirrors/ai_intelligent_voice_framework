/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef INTELL_VOICE_DEFINITIONS_H
#define INTELL_VOICE_DEFINITIONS_H
#include <string>
#include "intell_voice_util.h"
namespace OHOS {
namespace IntellVoiceEngine {
constexpr int32_t VOICE_WAKEUP_MODEL_UUID = 1;
constexpr int32_t PROXIMAL_WAKEUP_MODEL_UUID = 2;
const std::string WAKEUP_KEY = "intell_voice_trigger_enabled";
const std::string WHISPER_KEY = "intell_voice_trigger_whisper";
const std::string SHORTWORD_KEY = "intell_voice_trigger_shortword";
const std::string DUALMIC_KEY = "dualmic_intell_voice_trigger_enabled";
const std::string SENSIBILITY_TEXT = "sensibility=";

const std::string KEY_GET_WAKEUP_FEATURE = "wakeup_features";
const std::string KEY_WAKEUP_VESRION = "WakeupVersion";
const std::string KEY_LANGUAGE = "Language";
const std::string KEY_AREA = "Area";
const std::string KEY_WAKEUP_PHRASE = "WakeupPhrase";
const std::string KEY_ENROLL_ENGINE_UID = "EnrollEngineUid";
const std::string KEY_SENSIBILITY = "Sensibility";
const std::string KEY_WAKEUP_DSP_FEATURE = "WakeupDspFeature";
const std::string KEY_WHISPER_VPR = "WhisperVpr";
const std::string KEY_DUALMIC_ENABLE = "dualMicEnable";

const std::string HEADSET_SERVICE_NAME = "tws_kws_service";
const std::string DEFAULT_MAC_ADDRESS = "default_mac_address_phone";
const std::string DEFAULT_SERVICE_NAME = "default_service_name_phone";

static const std::string WHISPER_MODEL_PATH_VDE =
    "/sys_prod/variant/region_comm/china/etc/intellvoice/wakeup/dsp/whisper_wakeup_dsp_config_vde";
static const std::string WHISPER_VPR_CONFIG_PATH =
    "/sys_prod/variant/region_comm/china/etc/intellvoice/wakeup/vpr/whisper/vpr_domainDict_04.dat";
static const std::string WHISPER_MODEL_PATH =
    "/sys_prod/variant/region_comm/china/etc/intellvoice/wakeup/dsp/whisper_wakeup_dsp_config";
static const std::string VAD_MODEL_PATH =
    "/sys_prod/variant/region_comm/china/etc/intellvoice/wakeup/ap/kws2_acousticsModel6.pb";
static const std::string WAKEUP_CONFIG_USER_PATH =
    "/sys_prod/variant/region_comm/china/etc/intellvoice/wakeup/ap/wakeup_config_user.json";
static const std::string WAKEUP_CONFIG_PATH =
    "/sys_prod/variant/region_comm/china/etc/intellvoice/wakeup/ap/wakeup_config.json";
#ifdef ONLY_FIRST_STAGE
static const std::string SINGLE_LEVEL_MODEL_CN_PATH =
    "/sys_prod/variant/region_comm/china/etc/intellvoice/wakeup/dsp/wakeup_dsp_config";
static const std::string SINGLE_LEVEL_MODEL_GB_PATH =
    "/sys_prod/variant/region_comm/oversea/etc/intellvoice/wakeup/dsp/gb_wakeup_dsp_config";
static const std::string SINGLE_LEVEL_MODEL_AE_PATH =
    "/sys_prod/variant/region_comm/oversea/etc/intellvoice/wakeup/dsp/ae_wakeup_dsp_config";
static const std::string SINGLE_LEVEL_MODEL_MY_PATH =
    "/sys_prod/variant/region_comm/oversea/etc/intellvoice/wakeup/dsp/my_wakeup_dsp_config";
static const std::string SINGLE_LEVEL_MODEL_MX_PATH =
    "/sys_prod/variant/region_comm/oversea/etc/intellvoice/wakeup/dsp/mx_wakeup_dsp_config";
static const std::string SINGLE_LEVEL_MODEL_SA_PATH =
    "/sys_prod/variant/region_comm/oversea/etc/intellvoice/wakeup/dsp/sa_wakeup_dsp_config";
static const std::string SINGLE_LEVEL_MODEL_CL_PATH =
    "/sys_prod/variant/region_comm/oversea/etc/intellvoice/wakeup/dsp/cl_wakeup_dsp_config";
#endif
static const std::string WAKEUP_FILE_PATH_PREFIX =
    "/data/service/el0/intelligent_voice/wakeup";
static const std::string PARENT_PATH_PREFIX = "../";
}  // namespace IntellVoice
namespace IntellVoiceTrigger {
    enum TriggerModelType {
        UNKNOWN_TYPE = -1,
        GENERIC_TYPE = 1,
        VOICE_WAKEUP_TYPE = 1,
        PROXIMAL_WAKEUP_TYPE = 2,
    };
} // namespace IntellVoice
}  // namespace OHOS
#endif