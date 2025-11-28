// include/app/data_service.h (修正版)

#ifndef DATA_SERVICE_H
#define DATA_SERVICE_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h> // 【新增】用于 time_t

// ... (API 宏定义保持不变) ...

// ------------------------------------
// 1. 天气数据结构体 (新增时间戳)
// ------------------------------------
typedef struct {
  float temperature;
  char weather_desc[32];
  char wind_scale[16];
  int weather_code;
  bool is_available;
  time_t last_updated_time; // 【新增】上次数据更新的时间戳
} weather_data_t;

/**
 * @brief 初始化数据服务模块 (初始化互斥锁)
 */
void data_service_init(void);

/**
 * @brief 尝试从本地缓存加载数据，并检查其是否新鲜（是否是今天的数据）。
 * @return true: 缓存有效且已加载到全局数据中; false: 缓存无效或不存在。
 */
bool data_service_load_cache(void);

/**
 * @brief 获取最新的天气数据 (同步阻塞调用，成功后会写入缓存)
 */
void data_service_fetch_weather(void);

/**
 * @brief 获取当前缓存的天气数据指针（非 NULL）
 */
weather_data_t *data_service_get_weather(void);

// ... (data_service_get_weather 保持不变) ...

#endif // DATA_SERVICE_H