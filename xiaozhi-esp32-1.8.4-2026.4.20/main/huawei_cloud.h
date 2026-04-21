#ifndef HUAWEI_CLOUD_H
#define HUAWEI_CLOUD_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void huawei_cloud_start(void);
bool huawei_cloud_is_connected(void);
int huawei_cloud_publish(const char *topic, const char *data);

#ifdef __cplusplus
}
#endif

#endif
