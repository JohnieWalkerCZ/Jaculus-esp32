#pragma once

#include "Profiler.hpp"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "include/Font/Font.hpp"
#include "include/Renderer.hpp"
#include "include/Shapes/Circle.hpp"
#include "include/Shapes/Collection.hpp"
#include "include/Shapes/LineSegment.hpp"
#include "include/Shapes/Point.hpp"
#include "include/Shapes/Polygon.hpp"
#include "include/Shapes/Rectangle.hpp"
#include "include/Shapes/RegularPolygon.hpp"
#include "include/Shapes/Shape.hpp"
#include "jac/device/logger.h"
#include "jac/machine/context.h"
#include "jac/machine/internal/declarations.h"
#include "quickjs.h"
#include "types.h"

#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/machine.h>
#include <jac/machine/values.h>

// ===================================
//      Type Conversions (ConvTraits)
// ===================================

template <> struct jac::ConvTraits<Color> {
    static Color from(ContextRef ctx, ValueWeak val) {
        if (JS_IsString(val.getVal())) {
            auto str = val.to<std::string>();
            if (str == "red")
                return Colors::RED;
            if (str == "green")
                return Colors::GREEN;
            if (str == "blue")
                return Colors::BLUE;
            if (str == "yellow")
                return Colors::YELLOW;
            if (str == "magenta")
                return Colors::MAGENTA;
            if (str == "cyan")
                return Colors::CYAN;
            if (str == "white")
                return Colors::WHITE;
            if (str == "black")
                return Colors::BLACK;
        }
        auto array = val.to<jac::ArrayWeak>();
        return Color(array.get(0).to<uint8_t>(), array.get(1).to<uint8_t>(),
                     array.get(2).to<uint8_t>(),
                     array.length() > 3 ? array.get(3).to<float>() : 1.0f);
    }

    static jac::Value to(ContextRef ctx, const Color &color) {
        // 1. Create empty array (Fix: create() takes only ctx)
        auto arr = jac::Array::create(ctx);

        // 2. Set elements (0, 1, 2, 3)
        // We use jac::toValue to ensure primitives are boxed correctly
        arr.set(0, jac::toValue(ctx, color.r));
        arr.set(1, jac::toValue(ctx, color.g));
        arr.set(2, jac::toValue(ctx, color.b));
        arr.set(3, jac::toValue(ctx, color.a));

        return arr;
    }
};

template <> struct jac::ConvTraits<ShapeParams> {
    static ShapeParams from(ContextRef ctx, ValueWeak val) {
        auto obj = val.to<jac::ObjectWeak>();
        return ShapeParams(obj.get<float>("x"), obj.get<float>("y"),
                           obj.get<Color>("color"),
                           obj.hasProperty("z") ? obj.get<float>("z") : 0);
    }
};

template <> struct jac::ConvTraits<CircleParams> {
    static CircleParams from(ContextRef ctx, ValueWeak val) {
        auto obj = val.to<jac::ObjectWeak>();
        return CircleParams(obj.get<float>("x"), obj.get<float>("y"),
                            obj.get<Color>("color"), obj.get<int>("radius"),
                            obj.hasProperty("fill") ? obj.get<bool>("fill")
                                                    : false,
                            obj.hasProperty("z") ? obj.get<float>("z") : 0);
    }
};

template <> struct jac::ConvTraits<RectangleParams> {
    static RectangleParams from(ContextRef ctx, ValueWeak val) {
        auto obj = val.to<jac::ObjectWeak>();
        return RectangleParams(
            obj.get<float>("x"), obj.get<float>("y"), obj.get<Color>("color"),
            obj.get<int>("width"), obj.get<int>("height"),
            obj.hasProperty("fill") ? obj.get<bool>("fill") : false,
            obj.hasProperty("z") ? obj.get<float>("z") : 0);
    }
};

template <> struct jac::ConvTraits<PolygonParams> {
    static PolygonParams from(ContextRef ctx, ValueWeak val) {
        auto obj = val.to<jac::ObjectWeak>();
        auto vertices_js = obj.get<jac::ArrayWeak>("vertices");
        std::vector<std::pair<int, int>> vertices;
        uint32_t len = vertices_js.length();
        vertices.reserve(len);
        for (uint32_t i = 0; i < len; ++i) {
            auto vertex_js = vertices_js.get(i).to<jac::ArrayWeak>();
            vertices.push_back(
                {vertex_js.get(0).to<int>(), vertex_js.get(1).to<int>()});
        }
        return PolygonParams(
            obj.get<float>("x"), obj.get<float>("y"), obj.get<Color>("color"),
            vertices, obj.hasProperty("fill") ? obj.get<bool>("fill") : false,
            obj.hasProperty("z") ? obj.get<float>("z") : 0);
    }
};

template <> struct jac::ConvTraits<LineSegmentParams> {
    static LineSegmentParams from(ContextRef ctx, ValueWeak val) {
        auto obj = val.to<jac::ObjectWeak>();
        return LineSegmentParams(
            obj.get<float>("x"), obj.get<float>("y"), obj.get<Color>("color"),
            obj.get<float>("x2"), obj.get<float>("y2"),
            obj.hasProperty("z") ? obj.get<float>("z") : 0);
    }
};

template <> struct jac::ConvTraits<RegularPolygonRadiusParams> {
    static RegularPolygonRadiusParams from(ContextRef ctx, ValueWeak val) {
        auto obj = val.to<jac::ObjectWeak>();
        return RegularPolygonRadiusParams(
            obj.get<float>("x"), obj.get<float>("y"), obj.get<Color>("color"),
            obj.get<int>("sides"), obj.get<int>("radius"),
            obj.hasProperty("fill") ? obj.get<bool>("fill") : false);
    }
};

template <> struct jac::ConvTraits<RegularPolygonSideParams> {
    static RegularPolygonSideParams from(ContextRef ctx, ValueWeak val) {
        auto obj = val.to<jac::ObjectWeak>();
        return RegularPolygonSideParams(
            obj.get<float>("x"), obj.get<float>("y"), obj.get<Color>("color"),
            obj.get<int>("sides"), obj.get<int>("sideLength"),
            obj.hasProperty("fill") ? obj.get<bool>("fill") : false,
            obj.hasProperty("z") ? obj.get<float>("z") : 0);
    }
};
// ===================================
//      ProtoBuilders
// ===================================
class TextureProtoBuilder : public jac::ProtoBuilder::Opaque<Texture>,
                            public jac::ProtoBuilder::Properties {
  public:
    static Texture *unwrap(jac::ContextRef ctx, jac::ValueWeak val) {
        return getOpaque(ctx, val);
    }

    static Texture *constructOpaque(jac::ContextRef ctx,
                                    std::vector<jac::ValueWeak> args) {
        return new Texture();
    }

    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        jac::FunctionFactory ff(ctx);

        proto.defineProperty(
            "load",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  std::string path) {
                Texture *self = getOpaque(ctx, thisVal);
                if (self) {
                    bool success = Texture::fromBMP(path, *self);
                    return jac::Value::from(ctx, success);
                }
                return jac::Value::from(ctx, false);
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "setWrapMode",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  std::string mode) {
                Texture *self = getOpaque(ctx, thisVal);
                if (self)
                    self->setWrapMode(mode);
                return jac::Value::undefined(ctx);
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "getWidth",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal) {
                Texture *self = getOpaque(ctx, thisVal);
                return jac::Value::from(ctx, self ? self->getWidth() : 0);
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "getHeight",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal) {
                Texture *self = getOpaque(ctx, thisVal);
                return jac::Value::from(ctx, self ? self->getHeight() : 0);
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "isValid",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal) {
                Texture *self = getOpaque(ctx, thisVal);
                return jac::Value::from(ctx, self ? self->isValid() : false);
            }),
            jac::PropFlags::Enumerable);
    }
};

class ShapeProtoBuilder : public jac::ProtoBuilder::Opaque<Shape>,
                          public jac::ProtoBuilder::Properties {

  private:
    static inline std::vector<JSClassID> derivedClassIDs;

    using ShapeGetter =
        std::function<jac::Value(jac::ContextRef ctx, Shape *shape)>;

    struct PropDef {
        const char *name;
        ShapeGetter getter;
    };

    static void registerGetters(jac::ContextRef ctx, jac::Object proto,
                                jac::FunctionFactory &ff,
                                const std::vector<PropDef> &props) {
        for (const auto &prop : props) {
            proto.defineProperty(
                prop.name,
                ff.newFunctionThis(
                    [getter = prop.getter](jac::ContextRef ctx,
                                           jac::ValueWeak thisVal) {
                        Shape *shape = unwrapShape(ctx, thisVal);
                        if (!shape)
                            return jac::Value::undefined(ctx);
                        return getter(ctx, shape);
                    }),
                jac::PropFlags::Enumerable);
        }
    }
    template <typename... Args> struct SetterDef {
        const char *name;
        std::function<void(Shape *, Args...)> func;
    };

    template <typename... Args>
    static void registerSetters(jac::ContextRef ctx, jac::Object proto,
                                jac::FunctionFactory &ff,
                                const std::vector<SetterDef<Args...>> &defs) {
        for (const auto &def : defs) {
            proto.defineProperty(
                def.name,
                ff.newFunctionThis([func = def.func](jac::ContextRef ctx,
                                                     jac::ValueWeak thisVal,
                                                     Args... args) {
                    Shape *shape = unwrapShape(ctx, thisVal);
                    if (!shape)
                        return;
                    func(shape, args...);
                }),
                jac::PropFlags::Enumerable);
        }
    }
    static void addBasicSetters(jac::ContextRef ctx, jac::Object proto,
                                jac::FunctionFactory &ff) {

        registerSetters<float>(
            ctx, proto, ff,
            {
                {"rotate", [](Shape *s, float v) { s->rotate(v); }},
                {"setRotationAngle",
                 [](Shape *s, float v) { s->setRotationAngle(v); }},
                {"setScaleX", [](Shape *s, float v) { s->setScaleX(v); }},
                {"setScaleY", [](Shape *s, float v) { s->setScaleY(v); }},
            });

        registerSetters<int>(ctx, proto, ff,
                             {
                                 {"setX", [](Shape *s, int v) { s->setX(v); }},
                                 {"setY", [](Shape *s, int v) { s->setY(v); }},
                                 {"setZ", [](Shape *s, int v) { s->setZ(v); }},
                             });

        registerSetters<int, int>(
            ctx, proto, ff,
            {
                {"setPosition",
                 [](Shape *s, int x, int y) { s->setPosition(x, y); }},
                {"setPivot", [](Shape *s, int x, int y) { s->setPivot(x, y); }},
            });

        registerSetters<float, float>(
            ctx, proto, ff,
            {
                {"translate",
                 [](Shape *s, float x, float y) { s->translate(x, y); }},
            });

        proto.defineProperty(
            "setScale",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  float scaleX, float scaleY,
                                  jac::ValueWeak originX,
                                  jac::ValueWeak originY) {
                Shape *shape = unwrapShape(ctx, thisVal);
                if (!shape)
                    return;

                float ox = originX.isUndefined() ? -1 : originX.to<float>();
                float oy = originY.isUndefined() ? -1 : originY.to<float>();
                shape->setScale(scaleX, scaleY, ox, oy);
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "setColor",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  jac::ValueWeak colorVal) {
                Color color = jac::fromValue<Color>(ctx, colorVal);
                unwrapShape(ctx, thisVal)->setColor(color);
            }),
            jac::PropFlags::Enumerable);
    }
    static void addTextureSetters(jac::ContextRef ctx, jac::Object proto,
                                  jac::FunctionFactory &ff) {

        registerSetters<float>(
            ctx, proto, ff,
            {
                {"setTextureRotation",
                 [](Shape *s, float v) { s->setTextureRotation(v); }},
                {"setUVScaleX", [](Shape *s, float v) { s->setUVScaleX(v); }},
                {"setUVScaleY", [](Shape *s, float v) { s->setUVScaleY(v); }},
                {"setUVOffsetX", [](Shape *s, float v) { s->setUVOffsetX(v); }},
                {"setUVOffsetY", [](Shape *s, float v) { s->setUVOffsetY(v); }},
                {"setUVRotation",
                 [](Shape *s, float v) { s->setUVRotation(v); }},
            });

        registerSetters<float, float>(
            ctx, proto, ff,
            {
                {"setTextureOffset",
                 [](Shape *s, float x, float y) { s->setTextureOffset(x, y); }},
                {"setTextureScale",
                 [](Shape *s, float x, float y) { s->setTextureScale(x, y); }},
            });

        registerSetters<bool>(
            ctx, proto, ff,
            {
                {"setFixTexture",
                 [](Shape *s, bool v) { s->setFixTexture(v); }},
            });
        proto.defineProperty(
            "setTexture",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  jac::ValueWeak texVal) {
                jac::Logger::debug("setTexture called");
                auto *shape = unwrapShape(ctx, thisVal);
                // Unwrap the Texture object
                Texture *tex = TextureProtoBuilder::unwrap(ctx, texVal);
                if (shape && tex) {
                    shape->setTexture(tex);
                }
                return jac::Value::undefined(ctx);
            }),
            jac::PropFlags::Enumerable);
    }

    static void addGetters(jac::ContextRef ctx, jac::Object proto,
                           jac::FunctionFactory &ff) {

        registerGetters(ctx, proto, ff,
                        {{"getX",
                          [](jac::ContextRef ctx, Shape *s) {
                              return jac::Value::from(ctx, s->getX());
                          }},
                         {"getY",
                          [](jac::ContextRef ctx, Shape *s) {
                              return jac::Value::from(ctx, s->getY());
                          }},
                         {"getZ",
                          [](jac::ContextRef ctx, Shape *s) {
                              return jac::Value::from(ctx, s->getZ());
                          }},
                         {"getRotationAngle",
                          [](jac::ContextRef ctx, Shape *s) {
                              return jac::Value::from(ctx,
                                                      s->getRotationAngle());
                          }},
                         {"getScaleX",
                          [](jac::ContextRef ctx, Shape *s) {
                              return jac::Value::from(ctx, s->getScaleX());
                          }},
                         {"getScaleY",
                          [](jac::ContextRef ctx, Shape *s) {
                              return jac::Value::from(ctx, s->getScaleY());
                          }},

                         {"getColor", [](jac::ContextRef ctx, Shape *s) {
                              return jac::toValue(ctx, s->getColor());
                          }}});
    }

    static void addColliderSetters(jac::ContextRef ctx, jac::Object proto,
                                   jac::FunctionFactory &ff) {
        proto.defineProperty(
            "intersects",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  jac::Object otherShapeVal) {
                Shape *shape1 = unwrapShape(ctx, thisVal);
                Shape *shape2 = unwrapShape(ctx, otherShapeVal);
                if (shape1 && shape2) {
                    return jac::toValue(ctx, shape1->intersects(shape2));
                }
                return jac::Value::undefined(ctx);
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "addCollider",
            ff.newFunctionThisVariadic([](jac::ContextRef ctx,
                                          jac::ValueWeak thisVal,
                                          std::vector<jac::ValueWeak> args) {
                Shape *shape = unwrapShape(ctx, thisVal);
                if (shape) {
                    Collider *collider = nullptr;

                    if (args.size() > 0) {
                        jac::ValueWeak colliderVal = args[0];

                        if (!colliderVal.isUndefined() &&
                            !colliderVal.isNull()) {
                            collider =
                                reinterpret_cast<Collider *>(JS_GetOpaque(
                                    colliderVal.getVal(),
                                    JS_GetClassID(colliderVal.getVal())));
                        }
                    }

                    shape->addCollider(collider);
                }
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "removeCollider",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal) {
                Shape *shape = unwrapShape(ctx, thisVal);
                if (shape) {
                    shape->removeCollider();
                }
            }),
            jac::PropFlags::Enumerable);
    }

  public:
    static void registerDerivedClass(JSClassID classId) {
        derivedClassIDs.push_back(classId);
    }

    static Shape *unwrapShape(jac::ContextRef ctx, jac::ValueWeak thisVal) {
        void *ptr = JS_GetOpaque(thisVal.getVal(), classId);

        if (ptr) {
            return static_cast<Shape *>(ptr);
        }

        for (auto derivedClassId : derivedClassIDs) {
            ptr = JS_GetOpaque(thisVal.getVal(), derivedClassId);
            if (ptr) {
                return static_cast<Shape *>(ptr);
            }
        }

        throw jac::Exception::create(jac::Exception::Type::TypeError,
                                     "Invalid Shape object");
    }

    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        jac::FunctionFactory ff(ctx);

        addBasicSetters(ctx, proto, ff);
        addTextureSetters(ctx, proto, ff);
        addGetters(ctx, proto, ff);
        addColliderSetters(ctx, proto, ff);
    };
};

#define SHAPE_BUILDER_BOILERPLATE(ClassName, ParamsType)                       \
    class ClassName##ProtoBuilder                                              \
        : public jac::ProtoBuilder::Opaque<ClassName>,                         \
          public jac::ProtoBuilder::Properties {                               \
      public:                                                                  \
        static ClassName *constructOpaque(jac::ContextRef ctx,                 \
                                          std::vector<jac::ValueWeak> args) {  \
            return new ClassName(jac::fromValue<ParamsType>(ctx, args[0]));    \
        }                                                                      \
        static void addProperties(jac::ContextRef ctx, jac::Object proto) {    \
            ShapeProtoBuilder::addProperties(ctx, proto);                      \
        }                                                                      \
    };

SHAPE_BUILDER_BOILERPLATE(Circle, CircleParams)
SHAPE_BUILDER_BOILERPLATE(Rectangle, RectangleParams)
SHAPE_BUILDER_BOILERPLATE(Polygon, PolygonParams)
SHAPE_BUILDER_BOILERPLATE(LineSegment, LineSegmentParams)
SHAPE_BUILDER_BOILERPLATE(Point, ShapeParams)

class CollectionProtoBuilder : public jac::ProtoBuilder::Opaque<Collection>,
                               public jac::ProtoBuilder::Properties {
  public:
    static Collection *constructOpaque(jac::ContextRef ctx,
                                       std::vector<jac::ValueWeak> args) {
        return new Collection(jac::fromValue<ShapeParams>(ctx, args[0]));
    }
    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        ShapeProtoBuilder::addProperties(ctx, proto);
        jac::FunctionFactory ff(ctx);
        proto.defineProperty(
            "add",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  jac::Object shapeVal) {
                auto &collection = *getOpaque(ctx, thisVal);
                auto shape = reinterpret_cast<Shape *>(JS_GetOpaque(
                    shapeVal.getVal(), JS_GetClassID(shapeVal.getVal())));
                if (shape) {
                    collection.addShape(shape);
                }
            }),
            jac::PropFlags::Enumerable);
    }
};

class RegularPolygonProtoBuilder
    : public jac::ProtoBuilder::Opaque<RegularPolygon>,
      public jac::ProtoBuilder::Properties {
  public:
    static RegularPolygon *constructOpaque(jac::ContextRef ctx,
                                           std::vector<jac::ValueWeak> args) {
        auto obj = args[0].to<jac::ObjectWeak>();
        if (obj.hasProperty("radius")) {
            return new RegularPolygon(
                jac::fromValue<RegularPolygonRadiusParams>(ctx, args[0]));
        } else {
            return new RegularPolygon(
                jac::fromValue<RegularPolygonSideParams>(ctx, args[0]));
        }
    }
    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        ShapeProtoBuilder::addProperties(ctx, proto);
    }
};

struct FrameBufferHolder {
    uint8_t *data;
    size_t size;
    size_t capacity;

    FrameBufferHolder(size_t cap) : capacity(cap) {
        data = (uint8_t *)heap_caps_malloc(capacity,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        if (!data) {
            jac::Logger::debug(
                "FrameBuffer: PSRAM allocation failed, using Internal RAM");
            data = (uint8_t *)malloc(capacity);
        }
        size = 0;
    }

    ~FrameBufferHolder() {
        if (data)
            free(data);
    }
};
class FrameBufferProtoBuilder
    : public jac::ProtoBuilder::Opaque<FrameBufferHolder>,
      public jac::ProtoBuilder::Properties {
  public:
    static FrameBufferHolder *
    constructOpaque(jac::ContextRef ctx, std::vector<jac::ValueWeak> args) {
        int w = args[0].to<int>();
        int h = args[1].to<int>();
        return new FrameBufferHolder(w * h * 8 * 5);
    }

    static FrameBufferHolder *unwrap(jac::ContextRef ctx, jac::ValueWeak val) {
        return getOpaque(ctx, val);
    }

    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        jac::FunctionFactory ff(ctx);

        proto.defineProperty(
            "clear",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal) {
                auto *fb = getOpaque(ctx, thisVal);
                if (fb) {
                    fb->size = 0; // Simply resetting size resets the buffer
                }
                return jac::Value::undefined(ctx);
            }),
            jac::PropFlags::Enumerable);
    }
};

class FontProtoBuilder : public jac::ProtoBuilder::Opaque<Font>,
                         public jac::ProtoBuilder::Properties {
  public:
    static Font *unwrap(jac::ContextRef ctx, jac::ValueWeak val) {
        return getOpaque(ctx, val);
    }

    static Font *constructOpaque(jac::ContextRef ctx,
                                 std::vector<jac::ValueWeak> args) {
        return new Font();
    }

    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        jac::FunctionFactory ff(ctx);

        proto.defineProperty(
            "getHeight",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal) {
                Font *font = unwrap(ctx, thisVal);
                if (!font)
                    return jac::Value::null(ctx);
                return jac::Value::from(ctx, font->getHeight());
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "getCharWidth",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  std::string charStr) {
                Font *font = unwrap(ctx, thisVal);
                if (!font || charStr.empty())
                    return jac::Value::null(ctx);
                return jac::Value::from(ctx, font->getCharWidth(charStr[0]));
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "getCharSpacing",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  std::string charStr) {
                Font *font = unwrap(ctx, thisVal);
                if (!font || charStr.empty())
                    return jac::Value::null(ctx);
                return jac::Value::from(ctx, font->getCharSpacing(charStr[0]));
            }),
            jac::PropFlags::Enumerable);
    }
};
class RendererHolder {
  private:
    ::Renderer *m_renderer;
    int m_width;
    int m_height;
    Pixels m_scratchPad;

  public:
    RendererHolder(int width, int height) : m_width(width), m_height(height) {
        m_renderer = new ::Renderer(width, height);
        m_scratchPad.reserve(20000);
    }
    ~RendererHolder() { delete m_renderer; }
    ::Renderer *getRenderer() { return m_renderer; }
    Pixels &getScratchPad() {
        m_scratchPad.clear();
        return m_scratchPad;
    }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
};

static void freeArrayBuffer(JSRuntime *rt, void *opaque, void *ptr) {
    free(ptr);
}

inline void logMemory() {
    size_t internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t spirit = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    jac::Logger::log(std::format("MEM: Internal: {} bytes | PSRAM: {} bytes",
                                 internal, spirit));
}

class RendererProtoBuilder : public jac::ProtoBuilder::Opaque<RendererHolder>,
                             public jac::ProtoBuilder::Properties {
  public:
    static RendererHolder *constructOpaque(jac::ContextRef ctx,
                                           std::vector<jac::ValueWeak> args) {
        int w = args[0].to<int>();
        int h = args[1].to<int>();
        if (w <= 0 || w > 512 || h <= 0 || h > 512) {
            w = 64;
            h = 64;
        }
        return new RendererHolder(w, h);
    }

    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        jac::FunctionFactory ff(ctx);
        proto.defineProperty(
            "render",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  jac::Object collectionVal,
                                  jac::Object bufferObj)
                                   -> jac::Value { // Accept Buffer as 3rd arg
                // 1. Get Renderer
                auto *holder = getOpaque(ctx, thisVal);
                if (!holder)
                    return jac::Value::undefined(ctx);
                // 2. Get Scene
                auto collection = reinterpret_cast<Collection *>(
                    JS_GetOpaque(collectionVal.getVal(),
                                 JS_GetClassID(collectionVal.getVal())));
                if (!collection)
                    return jac::Value::undefined(ctx);
                // 3. Get Reuseable Buffer (The Fix)
                // We access the C++ object directly from the JS object
                // passed in
                auto *fb = FrameBufferProtoBuilder::unwrap(ctx, bufferObj);
                if (!fb || !fb->data) {
                    jac::Logger::error("Renderer: Invalid FrameBuffer passed");
                    return jac::Value::undefined(ctx);
                }

                int w = holder->getWidth();
                int h = holder->getHeight();
                logMemory();
                Pixels &pixels = holder->getScratchPad();
                holder->getRenderer()->render(pixels, {collection},
                                              {w, h, true});
                size_t offset = 0;
                uint8_t *raw = fb->data;

                size_t maxBytes = fb->capacity;

                for (const auto &p : pixels) {
                    if (offset + 8 > maxBytes)
                        break;

                    int16_t x = (int16_t)p.x;
                    raw[offset++] = x & 0xFF;
                    raw[offset++] = (x >> 8) & 0xFF;
                    int16_t y = (int16_t)p.y;
                    raw[offset++] = y & 0xFF;
                    raw[offset++] = (y >> 8) & 0xFF;
                    raw[offset++] = p.color.r;
                    raw[offset++] = p.color.g;
                    raw[offset++] = p.color.b;
                    raw[offset++] = (uint8_t)(p.color.a * 255);
                }
                fb->size = offset;

                return jac::Value::undefined(ctx);
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "drawText",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  jac::Object bufferObj, std::string text,
                                  int x, int y, jac::ValueWeak fontValue,
                                  jac::ValueWeak colorValue,
                                  bool wrap) -> jac::Value {
                auto *holder = getOpaque(ctx, thisVal);
                if (!holder)
                    return jac::Value::undefined(ctx);

                auto *fb = FrameBufferProtoBuilder::unwrap(ctx, bufferObj);
                if (!fb || !fb->data)
                    return jac::Value::undefined(ctx);

                Font *fontPtr = FontProtoBuilder::unwrap(ctx, fontValue);
                const Font &font =
                    (fontPtr != nullptr) ? *fontPtr : defaultFont;
                Color color = jac::fromValue<Color>(ctx, colorValue);

                Pixels &pixels = holder->getScratchPad();
                pixels.clear();

                holder->getRenderer()->drawText(pixels, text, x, y, font, color,
                                                wrap);

                size_t offset = fb->size;

                uint8_t *raw = fb->data;
                size_t maxBytes = fb->capacity;

                for (const auto &p : pixels) {
                    if (offset + 8 > maxBytes) {
                        jac::Logger::error("Renderer: FrameBuffer overflow");
                        break;
                    }

                    int16_t px = (int16_t)p.x;
                    raw[offset++] = px & 0xFF;
                    raw[offset++] = (px >> 8) & 0xFF;
                    int16_t py = (int16_t)p.y;
                    raw[offset++] = py & 0xFF;
                    raw[offset++] = (py >> 8) & 0xFF;
                    raw[offset++] = p.color.r;
                    raw[offset++] = p.color.g;
                    raw[offset++] = p.color.b;
                    raw[offset++] = (uint8_t)(p.color.a * 255);
                }

                fb->size = offset;

                return jac::Value::undefined(ctx);
            }),
            jac::PropFlags::Enumerable);
    }
};

// ===================================
//      Main Feature Class
// ===================================

template <class Next> class RendererFeature : public Next {
  public:
    using ShapeClass = jac::Class<ShapeProtoBuilder>;

    using RendererClass = jac::Class<RendererProtoBuilder>;
    using CollectionClass = jac::Class<CollectionProtoBuilder>;
    using CircleClass = jac::Class<CircleProtoBuilder>;
    using RectangleClass = jac::Class<RectangleProtoBuilder>;
    using PolygonClass = jac::Class<PolygonProtoBuilder>;
    using LineSegmentClass = jac::Class<LineSegmentProtoBuilder>;
    using PointClass = jac::Class<PointProtoBuilder>;
    using RegularPolygonClass = jac::Class<RegularPolygonProtoBuilder>;
    using FrameBufferClass = jac::Class<FrameBufferProtoBuilder>;
    using FontClass = jac::Class<FontProtoBuilder>;
    using TextureClass = jac::Class<TextureProtoBuilder>;

    RendererFeature() {
        RendererClass::init("Renderer");

        ShapeClass::init("Shape");

        CollectionClass::init("Collection");
        CircleClass::init("Circle");
        RectangleClass::init("Rectangle");
        PolygonClass::init("Polygon");
        LineSegmentClass::init("LineSegment");
        PointClass::init("Point");
        RegularPolygonClass::init("RegularPolygon");

        FontClass::init("Font");
        FrameBufferClass::init("FrameBuffer");
        TextureClass::init("Texture");

        using ShapePB = ShapeProtoBuilder;
        ShapePB::registerDerivedClass(CircleClass::getClassId());
        ShapePB::registerDerivedClass(RectangleClass::getClassId());
        ShapePB::registerDerivedClass(PolygonClass::getClassId());
        ShapePB::registerDerivedClass(LineSegmentClass::getClassId());
        ShapePB::registerDerivedClass(PointClass::getClassId());
        ShapePB::registerDerivedClass(CollectionClass::getClassId());
        ShapePB::registerDerivedClass(RegularPolygonClass::getClassId());
    }

    void initialize() {
        Next::initialize();
        jac::Module &rendererModule = this->newModule("renderer");
        rendererModule.addExport(
            "Renderer", RendererClass::getConstructor(this->context()));

        rendererModule.addExport("Font",
                                 FontClass::getConstructor(this->context()));
        rendererModule.addExport("Texture",
                                 TextureClass::getConstructor(this->context()));

        jac::Module &shapesModule = this->newModule("shapes");
        shapesModule.addExport(
            "Collection", CollectionClass::getConstructor(this->context()));
        shapesModule.addExport("Circle",
                               CircleClass::getConstructor(this->context()));
        shapesModule.addExport("Rectangle",
                               RectangleClass::getConstructor(this->context()));
        shapesModule.addExport("Polygon",
                               PolygonClass::getConstructor(this->context()));
        shapesModule.addExport(
            "LineSegment", LineSegmentClass::getConstructor(this->context()));
        shapesModule.addExport("Point",
                               PointClass::getConstructor(this->context()));
        shapesModule.addExport(
            "RegularPolygon",
            RegularPolygonClass::getConstructor(this->context()));

        rendererModule.addExport(
            "FrameBuffer", FrameBufferClass::getConstructor(this->context()));
    }
};
