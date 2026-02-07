set(JAC_ESP32_VERSION "0.0.22")

idf_component_register(
    SRCS "main.cpp" "platform/espWifi.cpp" "platform/espNvsKeyValue.cpp"
        "espFeatures/gridui/gridUiFeature.cpp" "espFeatures/gridui/widgets/_common.cpp"
        "espFeatures/renderer/rendererFeature.cpp"
        "espFeatures/renderer/src/Texture.cpp"
        "espFeatures/renderer/src/Font.cpp"
        "espFeatures/renderer/src/Profiler.cpp"
        "espFeatures/renderer/src/Renderer.cpp"
        "espFeatures/renderer/src/Shapes/Collection.cpp"
        "espFeatures/renderer/src/Shapes/Rectangle.cpp"
        "espFeatures/renderer/src/Shapes/RegularPolygon.cpp"
        "espFeatures/renderer/src/Shapes/Point.cpp"
        "espFeatures/renderer/src/Shapes/Shape.cpp"
        "espFeatures/renderer/src/Shapes/LineSegment.cpp"
        "espFeatures/renderer/src/Shapes/Polygon.cpp"
        "espFeatures/renderer/src/Shapes/Circle.cpp"
        "espFeatures/renderer/src/Collider.cpp"
        "espFeatures/hub75/hub75Feature.cpp"
        "../components/common/types.cpp"
    INCLUDE_DIRS "" "espFeatures/renderer/include" "espFeatures/hub75" "../components/common/include"
    REQUIRES jac-dcore jac-machine jac-link
             driver pthread spiffs vfs fatfs
             SmartLeds esp_timer Esp32-RBGridUI
			 ESP32-HUB75-MatrixPanel-DMA
             common esp_timer
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC NO_GFX)

target_compile_definitions(${COMPONENT_LIB} PRIVATE JAC_ESP32_VERSION="${JAC_ESP32_VERSION}" SPIRAM_DMA_BUFFER)

target_compile_options(${COMPONENT_LIB} PRIVATE "-mtext-section-literals")

add_subdirectory(resources)
target_link_libraries(${COMPONENT_LIB} PRIVATE resources)
