#include "examples/LittlefsTest.hpp"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "LittlefsTest";

void listFilesRecursive(const char *basePath, int depth) {
    char path[512];
    struct dirent *entry;
    DIR *dir = opendir(basePath);

    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", basePath);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char indent[32] = "";
        for (int i = 0; i < depth; i++) {
            strcat(indent, "  ");
        }

        int needed =
            snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
        if (needed >= sizeof(path)) {
            ESP_LOGW(TAG, "%s📁 %s/ (path too long, truncating)", indent,
                     entry->d_name);
        }

        if (entry->d_type == DT_DIR) {
            ESP_LOGI(TAG, "%s📁 %s/", indent, entry->d_name);
            listFilesRecursive(path, depth + 1);
        } else {
            FILE *f = fopen(path, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fclose(f);
                ESP_LOGI(TAG, "%s📄 %s (%ld bytes)", indent, entry->d_name,
                         size);
            } else {
                ESP_LOGI(TAG, "%s📄 %s", indent, entry->d_name);
            }
        }
    }
    closedir(dir);
}

void runLittlefsTest() {
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {};
    conf.base_path = "/lfs";
    conf.partition_label = "lfs";
    conf.format_if_mount_failed = false;
    conf.dont_mount = false;
    conf.read_only = false;
    conf.grow_on_mount = false;

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)",
                     esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)",
                 esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d bytes, used: %d bytes", total,
                 used);
        ESP_LOGI(TAG, "Free space: %d bytes", total - used);
    }

    ESP_LOGI(TAG, "=== Listing all files in LittleFS ===");
    listFilesRecursive("/lfs", 0);
    ESP_LOGI(TAG, "=== File listing complete ===");

    ESP_LOGI(TAG, "LittleFS test complete!");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
