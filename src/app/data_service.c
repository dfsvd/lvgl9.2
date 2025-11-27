// src/app/data_service.c

#include "data_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  // 用于文件操作
#include <time.h>
#include <unistd.h>

#include "app/network.h"
#include "cJSON.h"
#define API_PROVINCE "广东"
#define API_PLACE    "从化" 
#define API_ID       "10010165"  
#define API_KEY      "8953ffca8a9bf572d2f6d81584d5cdd2" 
#define WEATHER_API_BASE_URL "https://cn.apihz.cn/api/tianqi/tqyb.php"
// ------------------------------------
// 【全局存储与线程安全】
// ------------------------------------
static pthread_mutex_t weather_mutex;
static weather_data_t g_current_weather = {0};

#define CACHE_FILE_PATH "/tmp/weather_cache.json"  // 缓存文件路径

// ------------------------------------
// 辅助函数：将天气描述转换为数字代码
// ------------------------------------
static int weather_to_code(const char* weather_str) {
        if (strstr(weather_str, "晴")) return 1;
        if (strstr(weather_str, "多云")) return 2;
        if (strstr(weather_str, "阴")) return 3;
        if (strstr(weather_str, "雨")) return 4;
        if (strstr(weather_str, "雪")) return 5;
        return 0;  // 默认：未知/其他
}

// ------------------------------------
// 辅助函数：将结构体数据写入 JSON 文件
// ------------------------------------
static void data_service_save_cache() {
        cJSON* root = cJSON_CreateObject();
        if (!root) return;

        // 【线程安全】在读取全局数据之前加锁
        pthread_mutex_lock(&weather_mutex);

        // 1. 保存所有字段
        cJSON_AddNumberToObject(root, "temperature",
                                g_current_weather.temperature);
        cJSON_AddStringToObject(root, "weather_desc",
                                g_current_weather.weather_desc);
        cJSON_AddStringToObject(root, "wind_scale",
                                g_current_weather.wind_scale);
        cJSON_AddNumberToObject(root, "weather_code",
                                g_current_weather.weather_code);
        cJSON_AddNumberToObject(root, "last_updated_time",
                                time(NULL));  // 保存当前时间戳

        // 2. 打印到字符串并写入文件
        char* json_str = cJSON_PrintUnformatted(root);
        if (json_str) {
                FILE* fp = fopen(CACHE_FILE_PATH, "w");
                if (fp) {
                        fprintf(fp, "%s", json_str);
                        fclose(fp);
                        printf(
                            "[DataService] Cache saved successfully to %s.\n",
                            CACHE_FILE_PATH);
                } else {
                        perror(
                            "[DataService] ERROR: Failed to open cache file "
                            "for writing");
                }
                free(json_str);
        }

        cJSON_Delete(root);
        pthread_mutex_unlock(&weather_mutex);
}

// ------------------------------------
// 核心函数：加载缓存并检查新鲜度
// ------------------------------------
bool data_service_load_cache(void) {
        FILE* fp = fopen(CACHE_FILE_PATH, "r");
        if (!fp) {
                printf("[DataService] Cache file not found or inaccessible.\n");
                return false;
        }

        // 1. 读取整个文件内容到缓冲区
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char* buffer = (char*)malloc(file_size + 1);
        if (!buffer) {
                fclose(fp);
                return false;
        }
        fread(buffer, 1, file_size, fp);
        buffer[file_size] = '\0';
        fclose(fp);

        // 2. 解析 JSON
        cJSON* root = cJSON_Parse(buffer);
        free(buffer);
        if (!root) {
                printf("[DataService] Cache JSON parse error.\n");
                return false;
        }

        cJSON* time_item = cJSON_GetObjectItem(root, "last_updated_time");
        time_t last_time =
            cJSON_IsNumber(time_item) ? (time_t)time_item->valuedouble : 0;

        // 3. 校验时间：判断是否是今天的数据
        time_t now = time(NULL);
        struct tm tm_cache, tm_now;

        // 使用线程安全版本的 localtime_r
        localtime_r(&last_time, &tm_cache);
        localtime_r(&now, &tm_now);

        // 校验：年和年中的天数是否相同
        bool is_fresh = (tm_cache.tm_year == tm_now.tm_year &&
                         tm_cache.tm_yday == tm_now.tm_yday);

        if (is_fresh) {
                // 4. 数据新鲜：加载数据到全局结构体
                pthread_mutex_lock(&weather_mutex);

                g_current_weather.temperature =
                    (float)cJSON_GetObjectItem(root, "temperature")
                        ->valuedouble;

                // 必须检查字段是否存在且为字符串，否则可能崩溃
                cJSON* desc = cJSON_GetObjectItem(root, "weather_desc");
                cJSON* wind = cJSON_GetObjectItem(root, "wind_scale");

                if (cJSON_IsString(desc) && cJSON_IsString(wind)) {
                        strncpy(g_current_weather.weather_desc,
                                desc->valuestring,
                                sizeof(g_current_weather.weather_desc) - 1);
                        strncpy(g_current_weather.wind_scale, wind->valuestring,
                                sizeof(g_current_weather.wind_scale) - 1);
                }

                g_current_weather.weather_code =
                    (int)cJSON_GetObjectItem(root, "weather_code")->valuedouble;
                g_current_weather.last_updated_time = last_time;
                g_current_weather.is_available = true;

                printf(
                    "[DataService] Cache loaded successfully (Last update: %s)",
                    ctime(&last_time));

                pthread_mutex_unlock(&weather_mutex);
        } else {
                printf(
                    "[DataService] Cache found but expired or from a different "
                    "day. Needs update.\n");
        }

        cJSON_Delete(root);
        return is_fresh;
}

// ------------------------------------
// 核心函数：fetch (修改：成功后保存缓存)
// ------------------------------------
void data_service_fetch_weather(void) {
        char url[512];
        char* json_string = NULL;
        cJSON* root = NULL;
        cJSON* nowinfo = NULL;

        snprintf(url, sizeof(url), "%s?id=%s&key=%s&sheng=%s&place=%s",
                 WEATHER_API_BASE_URL, API_ID, API_KEY, API_PROVINCE,
                 API_PLACE);

        json_string = network_fetch_data(url);

        if (json_string == NULL || strlen(json_string) < 10) {
                printf(
                    "[DataService] Error: Failed to fetch data or empty "
                    "response.\n");
                goto cleanup;
        }

        root = cJSON_Parse(json_string);
        if (root == NULL) {
                printf("[DataService] Error: JSON parsing failed.\n");
                goto cleanup;
        }

        cJSON* code_item = cJSON_GetObjectItem(root, "code");
        if (!cJSON_IsNumber(code_item) || code_item->valueint != 200) {
                // API 错误，只更新 is_available 状态
                pthread_mutex_lock(&weather_mutex);
                g_current_weather.is_available = false;
                pthread_mutex_unlock(&weather_mutex);
                goto cleanup;
        }

        // 提取实时数据 (位于嵌套的 "nowinfo" 对象中)
        nowinfo = cJSON_GetObjectItem(root, "nowinfo");
        cJSON* temp_item = cJSON_GetObjectItem(nowinfo, "temperature");
        cJSON* wind_item = cJSON_GetObjectItem(nowinfo, "windScale");
        cJSON* weather1_item = cJSON_GetObjectItem(root, "weather1");

        // 【线程安全】在更新全局数据前加锁
        pthread_mutex_lock(&weather_mutex);

        if (cJSON_IsNumber(temp_item) && cJSON_IsString(weather1_item)) {
                // 成功提取数据，更新全局结构体
                g_current_weather.temperature = (float)temp_item->valuedouble;
                g_current_weather.is_available = true;

                strncpy(g_current_weather.weather_desc,
                        weather1_item->valuestring,
                        sizeof(g_current_weather.weather_desc) - 1);

                if (cJSON_IsString(wind_item)) {
                        strncpy(g_current_weather.wind_scale,
                                wind_item->valuestring,
                                sizeof(g_current_weather.wind_scale) - 1);
                } else {
                        strcpy(g_current_weather.wind_scale, "未知");
                }

                g_current_weather.weather_code =
                    weather_to_code(g_current_weather.weather_desc);
                g_current_weather.last_updated_time =
                    time(NULL);  // 记录请求成功的时间

                printf(
                    "[DataService] API Fetch Success: Temp=%.1f, Weather=%s\n",
                    g_current_weather.temperature,
                    g_current_weather.weather_desc);

        } else {
                printf(
                    "[DataService] Error: Missing required fields in JSON.\n");
                g_current_weather.is_available = false;
        }

        // 【线程安全】在离开前解锁
        pthread_mutex_unlock(&weather_mutex);

        // 【新增】如果数据有效，保存到缓存
        if (g_current_weather.is_available) {
                data_service_save_cache();
        }

cleanup:
        if (root) cJSON_Delete(root);
        if (json_string) free(json_string);
}

weather_data_t* data_service_get_weather(void) { return &g_current_weather; }

void data_service_init(void) {
        // 初始化互斥锁
        pthread_mutex_init(&weather_mutex, NULL);

        // 初始化数据结构
        memset(&g_current_weather, 0, sizeof(weather_data_t));
        g_current_weather.is_available = false;
        strcpy(g_current_weather.weather_desc, "加载中...");

        // 首次获取数据 (在 UI 启动逻辑中处理缓存加载，这里只初始化)
}