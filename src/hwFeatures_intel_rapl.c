#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <hwFeatures_types.h>
#include <likwid_device_types.h>
#include <error.h>
#include <hwFeatures_intel.h>
#include <hwFeatures_common.h>
#include <hwFeatures_intel_rapl.h>
#include <access.h>
#include <registers.h>


typedef struct {
    double powerUnit;
    double energyUnit;
    double timeUnit;
} IntelRaplDomainInfo;

IntelRaplDomainInfo intel_rapl_pkg_info = {0, 0, 0};
IntelRaplDomainInfo intel_rapl_dram_info = {0, 0, 0};
IntelRaplDomainInfo intel_rapl_psys_info = {0, 0, 0};
IntelRaplDomainInfo intel_rapl_pp0_info = {0, 0, 0};
IntelRaplDomainInfo intel_rapl_pp1_info = {0, 0, 0};

static int intel_rapl_register_test(uint32_t reg)
{
    int err = 0;
    int valid = 0;
    CpuTopology_t topo = NULL;

    err = topology_init();
    if (err < 0)
    {
        return 0;
    }
    topo = get_cpuTopology();
    for (int i = 0; i < topo->numSockets; i++)
    {
        for (int j = 0; j < topo->numHWThreads; j++)
        {
            uint64_t data = 0;
            HWThread* t = &topo->threadPool[j];
            if (t->packageId == i)
            {
                err = HPMread(t->apicId, MSR_DEV, reg, &data);
                if (err == 0) valid++;
                break;
            }
        }
    }
    return valid == topo->numSockets;
}

static int hwFeatures_intel_rapl_energy_status_getter(LikwidDevice_t device, char** value, uint32_t reg, IntelRaplDomainInfo* info)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data = extractBitField(data, 32, 0);
    data = (uint64_t)(((double)data) * info->energyUnit);

    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_energy_limit_1_getter(LikwidDevice_t device, char** value, uint32_t reg, IntelRaplDomainInfo* info)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data = extractBitField(data, 15, 0);
    data = (uint64_t)(((double)data) * info->powerUnit);

    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_energy_limit_1_setter(LikwidDevice_t device, char** value, uint32_t reg, IntelRaplDomainInfo* info)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    uint64_t limit = 0x0ULL;
    if ((!device) || (!value) || (reg == 0x0) || (!info) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }
    err = _string_to_uint64(value, &limit);
    if (err < 0)
    {
        return err;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data &= ~(0x7FFF);
    limit = (uint64_t)(((double)limit) / info->powerUnit);
    data |= (limit & 0x7FFF);

    err = HPMwrite(device->id.simple.id, MSR_DEV, reg, data);
    if (err < 0)
    {
        return err;
    }

    return 0;
}

static int hwFeatures_intel_rapl_energy_limit_1_time_getter(LikwidDevice_t device, char** value, uint32_t reg, IntelRaplDomainInfo* info)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    uint64_t y = extractBitField(data, 5, 17);
    uint64_t z = extractBitField(data, 2, 22);
    data = 0x0ULL;
    data = (1 << y) * info->timeUnit;
    data *= (uint64_t)((1.0 + (((double)z) / 4.0)));
    //data = (uint64_t)(((double)data) * intel_rapl_pkg_info.timeUnit);

    return _uint64_to_string(data, value);
}

/*static int hwFeatures_intel_rapl_energy_limit_1_time_setter(LikwidDevice_t device, char* value, uint32_t reg, IntelRaplDomainInfo* info)*/
/*{*/
/*    int err = 0;*/
/*    uint64_t data = 0x0ULL;*/
/*    uint64_t time = 0x0ULL;*/
/*    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))*/
/*    {*/
/*        return -EINVAL;*/
/*    }*/

/*    err = _string_to_uint64(value, &time);*/
/*    if (err < 0)*/
/*    {*/
/*        return err;*/
/*    }*/

/*    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);*/
/*    if (err < 0)*/
/*    {*/
/*        return err;*/
/*    }*/
/*    uint64_t y = (time & 0x1F);*/
/*    uint64_t y = ((time >> 5) & 0x3);*/
/*    */
/*    data &= ~(0x7F << 17);*/

/*    time = (uint64_t)(((double)z) * 4) + 1.0);*/
/*    */
/*    */
/*    uint64_t y = extractBitField(data, 5, 17);*/
/*    uint64_t z = extractBitField(data, 2, 22);*/
/*    data = 0x0ULL;*/
/*    data = (1 << y) * info->timeUnit;*/
/*    data *= (uint64_t)((1.0 + (((double)z) / 4.0)));*/
/*    //data = (uint64_t)(((double)data) * intel_rapl_pkg_info.timeUnit);*/

/*    return _uint64_to_string(data, value);*/
/*}*/

static int hwFeatures_intel_rapl_energy_limit_2_getter(LikwidDevice_t device, char** value, uint32_t reg, IntelRaplDomainInfo* info)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data = extractBitField(data, 15, 32);
    data = (uint64_t)(((double)data) * info->powerUnit);

    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_energy_limit_2_setter(LikwidDevice_t device, char** value, uint32_t reg, IntelRaplDomainInfo* info)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    uint64_t limit = 0x0ULL;
    if ((!device) || (!value) || (reg == 0x0) || (!info) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }
    err = _string_to_uint64(value, &limit);
    if (err < 0)
    {
        return err;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data &= ~(0x7FFF << 15);
    limit = (uint64_t)(((double)limit) / info->powerUnit);
    data |= (limit << 15);

    err = HPMwrite(device->id.simple.id, MSR_DEV, reg, data);
    if (err < 0)
    {
        return err;
    }

    return 0;
}

static int hwFeatures_intel_rapl_energy_limit_2_time_getter(LikwidDevice_t device, char** value, uint32_t reg, IntelRaplDomainInfo* info)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    uint64_t y = extractBitField(data, 5, 49);
    uint64_t z = extractBitField(data, 2, 54);
    data = 0x0ULL;
    data = (1 << y) * info->timeUnit;
    data *= (uint64_t)((1.0 + (((double)z) / 4.0)));
    //data = (uint64_t)(((double)data) * intel_rapl_pkg_info.timeUnit);

    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_info_tdp(LikwidDevice_t device, char** value, uint32_t reg)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data = extractBitField(data, 15, 0);
    data = (uint64_t)(((double)data) * intel_rapl_pkg_info.powerUnit);
    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_info_min_power(LikwidDevice_t device, char** value, uint32_t reg)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data = extractBitField(data, 15, 16);
    data = (uint64_t)(((double)data) * intel_rapl_pkg_info.powerUnit);
    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_info_max_power(LikwidDevice_t device, char** value, uint32_t reg)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data = extractBitField(data, 15, 32);
    data = (uint64_t)(((double)data) * 100000 * intel_rapl_pkg_info.powerUnit);
    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_info_max_time(LikwidDevice_t device, char** value, uint32_t reg)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data = extractBitField(data, 7, 48);
    data = (uint64_t)(((double)data) * intel_rapl_pkg_info.timeUnit);
    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_policy_getter(LikwidDevice_t device, char** value, uint32_t reg)
{
    int err = 0;
    uint64_t data = 0x0ULL;
    if ((!device) || (!value) || (device->type != DEVICE_TYPE_SOCKET))
    {
        return -EINVAL;
    }

    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data = extractBitField(data, 5, 0);
    return _uint64_to_string(data, value);
}

static int hwFeatures_intel_rapl_policy_setter(LikwidDevice_t device, char* value, uint32_t reg)
{
    int err = 0;
    uint64_t policy = 0x0ULL;
    err = _string_to_uint64(value, &policy);
    if (err < 0)
    {
        return err;
    }
    err = HPMread(device->id.simple.id, MSR_DEV, reg, &data);
    if (err < 0)
    {
        return err;
    }
    data &= ~(0x1F);
    data |= policy;
    err = HPMwrit(device->id.simple.id, MSR_DEV, reg, data);
    if (err < 0)
    {
        return err;
    }
    return 0;
}


/*********************************************************************************************************************/
/*                          Intel RAPL (PKG domain)                                                                  */
/*********************************************************************************************************************/

int intel_rapl_pkg_test()
{
    int err = 0;
    int valid = 0;
    CpuTopology_t topo = NULL;

    err = topology_init();
    if (err < 0)
    {
        return 0;
    }
    topo = get_cpuTopology();
    for (int i = 0; i < topo->numSockets; i++)
    {
        for (int j = 0; j < topo->numHWThreads; j++)
        {
            uint64_t data = 0;
            HWThread* t = &topo->threadPool[j];
            if (t->packageId == i)
            {
                err = HPMread(t->apicId, MSR_DEV, MSR_RAPL_POWER_UNIT, &data);
                if (err == 0) valid++;
                if (intel_rapl_pkg_info.powerUnit == 0 && intel_rapl_pkg_info.energyUnit == 0 && intel_rapl_pkg_info.timeUnit == 0)
                {
                    intel_rapl_pkg_info.powerUnit = 1000000 / (1 << (data & 0xF));
                    intel_rapl_pkg_info.energyUnit = 1.0 / (1 << ((data >> 8) & 0x1F));
                    intel_rapl_pkg_info.timeUnit = 1000000 / (1 << ((data >> 16) & 0xF));
                }
                break;
            }
        }
    }
    printf("intel_rapl_pkg_test returns %d\n", valid == topo->numSockets);
    return valid == topo->numSockets;
}


int hwFeatures_intel_pkg_energy_status_test()
{
    return intel_rapl_register_test(MSR_PKG_ENERGY_STATUS);
}


int hwFeatures_intel_pkg_energy_status_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_status_getter(device, value, MSR_PKG_ENERGY_STATUS, &intel_rapl_pkg_info);
}

int hwFeatures_intel_pkg_energy_limit_test()
{
    return intel_rapl_register_test(MSR_PKG_RAPL_POWER_LIMIT);
}

int hwFeatures_intel_pkg_energy_limit_1_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_getter(device, value, MSR_PKG_RAPL_POWER_LIMIT, &intel_rapl_pkg_info);
}
int hwFeatures_intel_pkg_energy_limit_1_setter(LikwidDevice_t device, char* value)
{
    return hwFeatures_intel_rapl_energy_limit_1_setter(device, value, MSR_PKG_RAPL_POWER_LIMIT, &intel_rapl_pkg_info);
}


int hwFeatures_intel_pkg_energy_limit_1_time_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_time_getter(device, value, MSR_PKG_RAPL_POWER_LIMIT, &intel_rapl_pkg_info);
}
/*int hwFeatures_intel_pkg_energy_limit_1_time_setter(LikwidDevice_t device, char* value);*/


int hwFeatures_intel_pkg_energy_limit_2_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_2_getter(device, value, MSR_PKG_RAPL_POWER_LIMIT, &intel_rapl_pkg_info);
}

int hwFeatures_intel_pkg_energy_limit_2_setter(LikwidDevice_t device, char* value)
{
    return hwFeatures_intel_rapl_energy_limit_2_setter(device, value, MSR_PKG_RAPL_POWER_LIMIT, &intel_rapl_pkg_info);
}


int hwFeatures_intel_pkg_energy_limit_2_time_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_2_time_getter(device, value, MSR_PKG_RAPL_POWER_LIMIT, &intel_rapl_pkg_info);
}

/*int hwFeatures_intel_pkg_energy_limit_2_time_setter(LikwidDevice_t device, char* value);*/

int hwFeatures_intel_pkg_info_test()
{
    return intel_rapl_register_test(MSR_PKG_POWER_INFO);
}

int hwFeatures_intel_pkg_info_tdp(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_info_tdp(device, value, MSR_PKG_POWER_INFO);
}

int hwFeatures_intel_pkg_info_min_power(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_info_min_power(device, value, MSR_PKG_POWER_INFO);
}

int hwFeatures_intel_pkg_info_max_power(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_info_max_power(device, value, MSR_PKG_POWER_INFO);
}

int hwFeatures_intel_pkg_info_max_time(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_info_max_time(device, value, MSR_PKG_POWER_INFO);
}


/*********************************************************************************************************************/
/*                          Intel RAPL (DRAM domain)                                                                 */
/*********************************************************************************************************************/

int intel_rapl_dram_test()
{
    int err = 0;
    int valid = 0;
    CpuTopology_t topo = NULL;
    CpuInfo_t info = NULL;

    err = topology_init();
    if (err < 0)
    {
        return 0;
    }
    topo = get_cpuTopology();
    info = get_cpuInfo();
    for (int i = 0; i < topo->numSockets; i++)
    {
        for (int j = 0; j < topo->numHWThreads; j++)
        {
            uint64_t data = 0;
            HWThread* t = &topo->threadPool[j];
            if (t->packageId == i)
            {
                err = HPMread(t->apicId, MSR_DEV, MSR_RAPL_POWER_UNIT, &data);
                if (err == 0) valid++;
                if (intel_rapl_dram_info.powerUnit == 0 && intel_rapl_dram_info.energyUnit == 0 && intel_rapl_dram_info.timeUnit == 0)
                {
                    intel_rapl_dram_info.powerUnit = 1000000 / (1 << (data & 0xF));
                    intel_rapl_dram_info.energyUnit = 1.0 / (1 << ((data >> 8) & 0x1F));
                    intel_rapl_dram_info.timeUnit = 1000000 / (1 << ((data >> 16) & 0xF));
                    if ((info->model == HASWELL_EP) ||
                        (info->model == HASWELL_M1) ||
                        (info->model == HASWELL_M2) ||
                        (info->model == BROADWELL_D) ||
                        (info->model == BROADWELL_E) ||
                        (info->model == SKYLAKEX) ||
                        (info->model == ICELAKEX1) ||
                        (info->model == ICELAKEX2) ||
                        (info->model == XEON_PHI_KNL) ||
                        (info->model == XEON_PHI_KML))
                    {
                        intel_rapl_dram_info.energyUnit = 15.3E-6;
                    }
                }
                break;
            }
        }
    }
    printf("MSR_RAPL_POWER_UNIT returns %d\n", valid == topo->numSockets);
    return valid == topo->numSockets;
}


int hwFeatures_intel_dram_energy_status_test()
{
    return intel_rapl_register_test(MSR_DRAM_ENERGY_STATUS);
}

int hwFeatures_intel_dram_energy_status_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_status_getter(device, value, MSR_DRAM_ENERGY_STATUS, &intel_rapl_dram_info);
}

int hwFeatures_intel_dram_energy_limit_test()
{
    return intel_rapl_register_test(MSR_DRAM_RAPL_POWER_LIMIT);
}

int hwFeatures_intel_dram_energy_limit_1_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_getter(device, value, MSR_DRAM_RAPL_POWER_LIMIT, &intel_rapl_dram_info);
}
int hwFeatures_intel_dram_energy_limit_1_setter(LikwidDevice_t device, char* value)
{
    return hwFeatures_intel_rapl_energy_limit_1_setter(device, value, MSR_DRAM_RAPL_POWER_LIMIT, &intel_rapl_dram_info);
}


int hwFeatures_intel_dram_energy_limit_1_time_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_time_getter(device, value, MSR_DRAM_RAPL_POWER_LIMIT, &intel_rapl_dram_info);
}
/*int hwFeatures_intel_dram_energy_limit_long_time_setter(LikwidDevice_t device, char* value);*/

int hwFeatures_intel_dram_info_test()
{
    return intel_rapl_register_test(MSR_DRAM_POWER_INFO);
}

int hwFeatures_intel_dram_info_tdp(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_info_tdp(device, value, MSR_DRAM_POWER_INFO);
}

int hwFeatures_intel_dram_info_min_power(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_info_min_power(device, value, MSR_DRAM_POWER_INFO);
}

int hwFeatures_intel_dram_info_max_power(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_info_max_power(device, value, MSR_DRAM_POWER_INFO);
}

int hwFeatures_intel_dram_info_max_time(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_info_max_time(device, value, MSR_DRAM_POWER_INFO);
}


/*********************************************************************************************************************/
/*                          Intel RAPL (PSYS or PLATFORM domain)                                                     */
/*********************************************************************************************************************/

int intel_rapl_psys_test()
{
    int err = 0;
    int valid = 0;
    CpuTopology_t topo = NULL;
    CpuInfo_t info = NULL;

    err = topology_init();
    if (err < 0)
    {
        return 0;
    }
    topo = get_cpuTopology();
    info = get_cpuInfo();
    for (int i = 0; i < topo->numSockets; i++)
    {
        for (int j = 0; j < topo->numHWThreads; j++)
        {
            uint64_t data = 0;
            HWThread* t = &topo->threadPool[j];
            if (t->packageId == i)
            {
                err = HPMread(t->apicId, MSR_DEV, MSR_RAPL_POWER_UNIT, &data);
                if (err == 0) valid++;
                if (intel_rapl_psys_info.powerUnit == 0 && intel_rapl_psys_info.energyUnit == 0 && intel_rapl_psys_info.timeUnit == 0)
                {
                    intel_rapl_psys_info.powerUnit = 1000000 / (1 << (data & 0xF));
                    intel_rapl_psys_info.energyUnit = 1.0 / (1 << ((data >> 8) & 0x1F));
                    intel_rapl_psys_info.timeUnit = 1000000 / (1 << ((data >> 16) & 0xF));
                }
                break;
            }
        }
    }
    printf("MSR_RAPL_POWER_UNIT returns %d\n", valid == topo->numSockets);
    return valid == topo->numSockets;
}


int hwFeatures_intel_psys_energy_status_test()
{
    return intel_rapl_register_test(MSR_PLATFORM_ENERGY_STATUS);
}


int hwFeatures_intel_psys_energy_status_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_status_getter(device, value, MSR_PLATFORM_ENERGY_STATUS, &intel_rapl_psys_info);
}

int hwFeatures_intel_psys_energy_limit_test()
{
    return intel_rapl_register_test(MSR_PLATFORM_POWER_LIMIT);
}

int hwFeatures_intel_psys_energy_limit_1_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_getter(device, value, MSR_PLATFORM_POWER_LIMIT, &intel_rapl_psys_info);
}
int hwFeatures_intel_psys_energy_limit_1_setter(LikwidDevice_t device, char* value)
{
    return hwFeatures_intel_rapl_energy_limit_1_setter(device, value, MSR_PLATFORM_POWER_LIMIT, &intel_rapl_psys_info);
}


int hwFeatures_intel_psys_energy_limit_1_time_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_time_getter(device, value, MSR_PLATFORM_POWER_LIMIT, &intel_rapl_psys_info);
}
/*int hwFeatures_intel_dram_energy_limit_1_time_setter(LikwidDevice_t device, char* value);*/


/*********************************************************************************************************************/
/*                          Intel RAPL (PP0 domain)                                                                  */
/*********************************************************************************************************************/

int intel_rapl_pp0_test()
{
    int err = 0;
    int valid = 0;
    CpuTopology_t topo = NULL;
    CpuInfo_t info = NULL;

    err = topology_init();
    if (err < 0)
    {
        return 0;
    }
    topo = get_cpuTopology();
    info = get_cpuInfo();
    for (int i = 0; i < topo->numSockets; i++)
    {
        for (int j = 0; j < topo->numHWThreads; j++)
        {
            uint64_t data = 0;
            HWThread* t = &topo->threadPool[j];
            if (t->packageId == i)
            {
                err = HPMread(t->apicId, MSR_DEV, MSR_RAPL_POWER_UNIT, &data);
                if (err == 0) valid++;
                if (intel_rapl_pp0_info.powerUnit == 0 && intel_rapl_pp0_info.energyUnit == 0 && intel_rapl_pp0_info.timeUnit == 0)
                {
                    intel_rapl_pp0_info.powerUnit = 1000000 / (1 << (data & 0xF));
                    intel_rapl_pp0_info.energyUnit = 1.0 / (1 << ((data >> 8) & 0x1F));
                    intel_rapl_pp0_info.timeUnit = 1000000 / (1 << ((data >> 16) & 0xF));
                }
                break;
            }
        }
    }
    printf("MSR_RAPL_POWER_UNIT returns %d\n", valid == topo->numSockets);
    return valid == topo->numSockets;
}


int hwFeatures_intel_pp0_energy_status_test()
{
    return intel_rapl_register_test(MSR_PP0_ENERGY_STATUS);
}


int hwFeatures_intel_pp0_energy_status_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_status_getter(device, value, MSR_PP0_ENERGY_STATUS, &intel_rapl_pp0_info);
}

int hwFeatures_intel_pp0_energy_limit_test()
{
    return intel_rapl_register_test(MSR_PP0_RAPL_POWER_LIMIT);
}

int hwFeatures_intel_pp0_energy_limit_1_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_getter(device, value, MSR_PP0_RAPL_POWER_LIMIT, &intel_rapl_pp0_info);
}
int hwFeatures_intel_pp0_energy_limit_1_setter(LikwidDevice_t device, char* value)
{
    return hwFeatures_intel_rapl_energy_limit_1_setter(device, value, MSR_PP0_RAPL_POWER_LIMIT, &intel_rapl_pp0_info);
}


int hwFeatures_intel_pp0_energy_limit_1_time_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_time_getter(device, value, MSR_PP0_RAPL_POWER_LIMIT, &intel_rapl_pp0_info);
}
/*int hwFeatures_intel_pp0_energy_limit_1_time_setter(LikwidDevice_t device, char* value);*/

int hwFeatures_intel_pp0_policy_test()
{
    return intel_rapl_register_test(MSR_PP0_ENERGY_POLICY);
}

int hwFeatures_intel_pp0_policy_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_policy_getter(device, value, MSR_PP0_ENERGY_POLICY);
}

int hwFeatures_intel_pp0_policy_setter(LikwidDevice_t device, char* value)
{
    return hwFeatures_intel_rapl_policy_setter(device, value, MSR_PP0_ENERGY_POLICY);
}


/*********************************************************************************************************************/
/*                          Intel RAPL (PP1 domain)                                                                  */
/*********************************************************************************************************************/

int intel_rapl_pp1_test()
{
    int err = 0;
    int valid = 0;
    CpuTopology_t topo = NULL;
    CpuInfo_t info = NULL;

    err = topology_init();
    if (err < 0)
    {
        return 0;
    }
    topo = get_cpuTopology();
    info = get_cpuInfo();
    for (int i = 0; i < topo->numSockets; i++)
    {
        for (int j = 0; j < topo->numHWThreads; j++)
        {
            uint64_t data = 0;
            HWThread* t = &topo->threadPool[j];
            if (t->packageId == i)
            {
                err = HPMread(t->apicId, MSR_DEV, MSR_RAPL_POWER_UNIT, &data);
                if (err == 0) valid++;
                if (intel_rapl_pp1_info.powerUnit == 0 && intel_rapl_pp1_info.energyUnit == 0 && intel_rapl_pp1_info.timeUnit == 0)
                {
                    intel_rapl_pp1_info.powerUnit = 1000000 / (1 << (data & 0xF));
                    intel_rapl_pp1_info.energyUnit = 1.0 / (1 << ((data >> 8) & 0x1F));
                    intel_rapl_pp1_info.timeUnit = 1000000 / (1 << ((data >> 16) & 0xF));
                }
                break;
            }
        }
    }
    printf("MSR_RAPL_POWER_UNIT returns %d\n", valid == topo->numSockets);
    return valid == topo->numSockets;
}


int hwFeatures_intel_pp1_energy_status_test()
{
    return intel_rapl_register_test(MSR_PP1_ENERGY_STATUS);
}

int hwFeatures_intel_pp1_energy_status_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_status_getter(device, value, MSR_PP1_ENERGY_STATUS, &intel_rapl_pp1_info);
}

int hwFeatures_intel_pp1_energy_limit_test()
{
    return intel_rapl_register_test(MSR_PP1_RAPL_POWER_LIMIT);
}

int hwFeatures_intel_pp1_energy_limit_1_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_getter(device, value, MSR_PP1_RAPL_POWER_LIMIT, &intel_rapl_pp1_info);
}
int hwFeatures_intel_pp1_energy_limit_1_setter(LikwidDevice_t device, char* value)
{
    return hwFeatures_intel_rapl_energy_limit_1_setter(device, value, MSR_PP1_RAPL_POWER_LIMIT, &intel_rapl_pp1_info);
}


int hwFeatures_intel_pp1_energy_limit_1_time_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_energy_limit_1_time_getter(device, value, MSR_PP1_RAPL_POWER_LIMIT, &intel_rapl_pp1_info);
}
/*int hwFeatures_intel_pp1_energy_limit_1_time_setter(LikwidDevice_t device, char* value);*/

int hwFeatures_intel_pp1_policy_test()
{
    return intel_rapl_register_test(MSR_PP1_ENERGY_POLICY);
}

int hwFeatures_intel_pp1_policy_getter(LikwidDevice_t device, char** value)
{
    return hwFeatures_intel_rapl_policy_getter(device, value, MSR_PP1_ENERGY_POLICY);
}

int hwFeatures_intel_pp1_policy_setter(LikwidDevice_t device, char* value)
{
    return hwFeatures_intel_rapl_policy_setter(device, value, MSR_PP1_ENERGY_POLICY);
}


/* Init function */

int hwFeatures_init_intel_rapl(_HWFeatureList* out)
{
    int err = 0;
    if (intel_rapl_pkg_test())
    {
        printf("Register Intel RAPL PKG domain\n");
        err = register_features(out, &intel_rapl_pkg_feature_list);
        if (err < 0)
        {
            printf("RAPL domain PKG not supported\n");
        }
    }
    if (intel_rapl_dram_test())
    {
        printf("Register Intel RAPL DRAM domain\n");
        err = register_features(out, &intel_rapl_dram_feature_list);
        if (err < 0)
        {
            printf("RAPL domain DRAM not supported\n");
        }
    }
    if (intel_rapl_pp0_test())
    {
        printf("Register Intel RAPL PP0 domain\n");
        err = register_features(out, &intel_rapl_pp0_feature_list);
        if (err < 0)
        {
            printf("RAPL domain PP0 not supported\n");
        }
    }
    if (intel_rapl_pp1_test())
    {
        printf("Register Intel RAPL PP1 domain\n");
        err = register_features(out, &intel_rapl_pp1_feature_list);
        if (err < 0)
        {
            printf("RAPL domain PP1 not supported\n");
        }
    }
    if (intel_rapl_psys_test())
    {
        printf("Register Intel RAPL PSYS domain\n");
        err = register_features(out, &intel_rapl_psys_feature_list);
        if (err < 0)
        {
            printf("RAPL domain PSYS not supported\n");
        }
    }
    return 0;
}
