#pragma once

#include "Circle.hpp"
#include "Collection.hpp"
#include "Font.hpp"
#include "LineSegment.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "Rectangle.hpp"
#include "RegularPolygon.hpp"
#include "Renderer.hpp"
#include "Shape.hpp"
#include "Utils.hpp"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "jac/device/logger.h"
#include "jac/machine/context.h"
#include "jac/machine/internal/declarations.h"
#include "quickjs.h"

#include <cstdint>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <memory>

// Reference:
// https://419.ecma-international.org/3.0/index.html#-15-display-class-pattern-pixel-format-values
size_t packColor(uint8_t *raw, const Color &p, int format) {
    uint8_t r = p.r;
    uint8_t g = p.g;
    uint8_t b = p.b;
    uint8_t a = (uint8_t)(p.a * 255);

    switch (format) {
    case 3: { // 1-bit monochrome
        raw[0] = (r + g + b) / 3 > 127 ? 1 : 0;
        return 1;
    }
    case 4: { // 4-bit grayscale
        uint8_t gray = (uint8_t)((0.299f * r + 0.587f * g + 0.114f * b));
        raw[0] = (gray >> 4) & 0x0F;
        return 1;
    }
    case 5: { // 8-bit grayscale
        raw[0] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
        return 1;
    }
    case 6: { // 8-bit RGB 3:3:2
        raw[0] = (r & 0xE0) | ((g >> 3) & 0x1C) | (b >> 6);
        return 1;
    }
    case 7: { // 16-bit RGB 5:6:5 Little-Endian
        uint16_t rgb = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        raw[0] = rgb & 0xFF;
        raw[1] = (rgb >> 8) & 0xFF;
        return 2;
    }
    case 8: { // 16-bit RGB 5:6:5 Big-Endian
        uint16_t rgb = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        raw[0] = (rgb >> 8) & 0xFF;
        raw[1] = rgb & 0xFF;
        return 2;
    }
    case 9: { // 24-bit RGB 8:8:8
        raw[0] = r;
        raw[1] = g;
        raw[2] = b;
        return 3;
    }
    case 10: { // 32-bit RGBA 8:8:8:8
        raw[0] = r;
        raw[1] = g;
        raw[2] = b;
        raw[3] = a;
        return 4;
    }
    case 12: { // 12-bit xRGB 4:4:4:4 (x is unused/alpha)
        raw[0] = (g & 0xF0) | (b >> 4);
        raw[1] = (a & 0xF0) | (r >> 4); // a used as 'x'
        return 2;
    }
    default:
        return 0;
    }
}

size_t packedColorSize(int format) {
    switch (format) {
    case 3:
    case 4:
    case 5:
    case 6:
        return 1;
    case 7:
    case 8:
    case 12:
        return 2;
    case 9:
        return 3;
    case 10:
        return 4;
    default:
        return 0;
    }
}

size_t writeDenseFramebuffer(uint8_t *raw, size_t maxBytes, int width, int height,
                             int format, const Pixels &pixels) {
    size_t bytesPerPixel = packedColorSize(format);
    if (bytesPerPixel == 0)
        return 0;

    size_t frameBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * bytesPerPixel;
    if (frameBytes > maxBytes)
        return 0;

    memset(raw, 0, frameBytes);

    for (const auto &p : pixels) {
        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)
            continue;

        size_t offset = (static_cast<size_t>(p.y) * static_cast<size_t>(width) + static_cast<size_t>(p.x)) * bytesPerPixel;
        packColor(&raw[offset], p.color, format);
    }

    return frameBytes;
}

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
        auto arr = jac::Array::create(ctx);

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
            obj.hasProperty("fill") ? obj.get<bool>("fill") : false,
            obj.hasProperty("z") ? obj.get<float>("z") : 0);
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

class ShapeProtoBuilder
    : public jac::ProtoBuilder::Opaque<std::shared_ptr<Shape>>,
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
                auto *shape = unwrapShape(ctx, thisVal);

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
                                  jac::ValueWeak otherShapeVal) {
                void *ptr1 = JS_GetOpaque(thisVal.getVal(),
                                          JS_GetClassID(thisVal.getVal()));

                void *ptr2 =
                    JS_GetOpaque(otherShapeVal.getVal(),
                                 JS_GetClassID(otherShapeVal.getVal()));

                if (ptr1 && ptr2) {
                    auto &s1 = *static_cast<std::shared_ptr<Shape> *>(ptr1);
                    auto &s2 = *static_cast<std::shared_ptr<Shape> *>(ptr2);

                    if (s1 && s2) {
                        return jac::toValue(ctx, s1->intersects(s2));
                    }
                }
                return jac::Value::from(ctx, false);
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
            return (*static_cast<std::shared_ptr<Shape> *>(ptr)).get();
        }

        for (auto derivedClassId : derivedClassIDs) {
            ptr = JS_GetOpaque(thisVal.getVal(), derivedClassId);
            if (ptr) {
                return (*static_cast<std::shared_ptr<Shape> *>(ptr)).get();
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
        : public jac::ProtoBuilder::Opaque<std::shared_ptr<Shape>>,            \
          public jac::ProtoBuilder::Properties {                               \
      public:                                                                  \
        static std::shared_ptr<Shape> *                                        \
        constructOpaque(jac::ContextRef ctx,                                   \
                        std::vector<jac::ValueWeak> args) {                    \
            return new std::shared_ptr<Shape>(                                 \
                new ClassName(jac::fromValue<ParamsType>(ctx, args[0])));      \
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

class CollectionProtoBuilder
    : public jac::ProtoBuilder::Opaque<std::shared_ptr<Collection>>,
      public jac::ProtoBuilder::Properties {
  public:
    static std::shared_ptr<Collection> *
    constructOpaque(jac::ContextRef ctx, std::vector<jac::ValueWeak> args) {
        auto rawPtr = new Collection(jac::fromValue<ShapeParams>(ctx, args[0]));
        return new std::shared_ptr<Collection>(rawPtr);
    }

    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        ShapeProtoBuilder::addProperties(ctx, proto);
        jac::FunctionFactory ff(ctx);

        proto.defineProperty(
            "add",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal,
                                  jac::Object shapeVal) {
                auto collectionPtr = getOpaque(ctx, thisVal);
                if (!collectionPtr || !*collectionPtr)
                    return;
                void *shapeOpaque = JS_GetOpaque(
                    shapeVal.getVal(), JS_GetClassID(shapeVal.getVal()));

                if (shapeOpaque) {
                    auto shapePtr =
                        reinterpret_cast<std::shared_ptr<Shape> *>(shapeOpaque);

                    if (shapePtr && *shapePtr) {
                        (*collectionPtr)->addShape(*shapePtr);
                    }
                }
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "clear",
            ff.newFunctionThis([](jac::ContextRef ctx, jac::ValueWeak thisVal) {
                auto collectionPtr = getOpaque(ctx, thisVal);
                if (collectionPtr && *collectionPtr) {
                    (*collectionPtr)->clear();
                }
                return jac::Value::undefined(ctx);
            }),
            jac::PropFlags::Enumerable);
    }
};

class RegularPolygonProtoBuilder
    : public jac::ProtoBuilder::Opaque<std::shared_ptr<Shape>>,
      public jac::ProtoBuilder::Properties {
  public:
    static std::shared_ptr<Shape> *
    constructOpaque(jac::ContextRef ctx, std::vector<jac::ValueWeak> args) {
        auto obj = args[0].to<jac::ObjectWeak>();
        Shape *rawShape;
        if (obj.hasProperty("radius")) {
            rawShape = new RegularPolygon(
                jac::fromValue<RegularPolygonRadiusParams>(ctx, args[0]));
        } else {
            rawShape = new RegularPolygon(
                jac::fromValue<RegularPolygonSideParams>(ctx, args[0]));
        }
        return new std::shared_ptr<Shape>(rawShape);
    }
    static void addProperties(jac::ContextRef ctx, jac::Object proto) {
        ShapeProtoBuilder::addProperties(ctx, proto);
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
            ff.newFunctionThisVariadic([](jac::ContextRef ctx,
                                          jac::ValueWeak thisVal,
                                          std::vector<jac::ValueWeak> args)
                                           -> jac::Value {
                if (args.size() < 2) {
                    jac::Logger::error("Renderer.render: Missing arguments "
                                       "(collection, buffer)");
                    return jac::Value::undefined(ctx);
                }

                auto *holder = getOpaque(ctx, thisVal);
                if (!holder)
                    return jac::Value::undefined(ctx);

                jac::ValueWeak collectionVal = args[0];
                auto collectionPtr =
                    reinterpret_cast<std::shared_ptr<Collection> *>(
                        JS_GetOpaque(collectionVal.getVal(),
                                     JS_GetClassID(collectionVal.getVal())));

                if (!collectionPtr || !*collectionPtr)
                    return jac::Value::undefined(ctx);

                jac::ValueWeak bufferObj = args[1];

                size_t maxBytes;
                uint8_t *raw =
                    JS_GetArrayBuffer(ctx, &maxBytes, bufferObj.getVal());
                if (!raw) {
                    jac::Logger::error("Renderer: Invalid ArrayBuffer passed");
                    return jac::Value::undefined(ctx);
                }

                bool antialias = (args.size() > 2) ? args[2].to<bool>() : true;

                int format = (args.size() > 3) ? args[3].to<int>() : 10;
                if (format < 3 || format == 11 || format > 12) {
                    jac::Logger::error("Renderer: Invalid color format");
                }

                int w = holder->getWidth();
                int h = holder->getHeight();
                Pixels &pixels = holder->getScratchPad();

                holder->getRenderer()->render(pixels, {*collectionPtr},
                                              {w, h, antialias});

                size_t frameBytes =
                    writeDenseFramebuffer(raw, maxBytes, w, h, format, pixels);
                if (frameBytes == 0) {
                    jac::Logger::error("Renderer.render: ArrayBuffer too small or invalid format");
                    return jac::Value::undefined(ctx);
                }

                return jac::Value(ctx, static_cast<int>(frameBytes));
            }),
            jac::PropFlags::Enumerable);

        proto.defineProperty(
            "drawText",
            ff.newFunctionThisVariadic([](jac::ContextRef ctx,
                                          jac::ValueWeak thisVal,
                                          std::vector<jac::ValueWeak> args)
                                           -> jac::Value {
                if (args.size() < 6) {
                    jac::Logger::error(
                        "Renderer.drawText: Missing arguments "
                        "(buffer, text, x, y, font, color, [wrap], [format])");
                    return jac::Value::undefined(ctx);
                }

                auto *holder = getOpaque(ctx, thisVal);
                if (!holder)
                    return jac::Value::undefined(ctx);

                jac::ValueWeak bufferObj = args[0];
                size_t maxBytes;
                uint8_t *raw =
                    JS_GetArrayBuffer(ctx, &maxBytes, bufferObj.getVal());
                if (!raw) {
                    jac::Logger::error(
                        "Renderer.drawText: Invalid ArrayBuffer passed");
                    return jac::Value::undefined(ctx);
                }

                std::string text = args[1].to<std::string>();
                int x = args[2].to<int>();
                int y = args[3].to<int>();

                Font *fontPtr = FontProtoBuilder::unwrap(ctx, args[4]);
                const Font &font =
                    (fontPtr != nullptr) ? *fontPtr : defaultFont;
                Color color = jac::fromValue<Color>(ctx, args[5]);

                bool wrap = (args.size() >= 7) ? args[6].to<bool>() : false;

                int format = (args.size() >= 8) ? args[7].to<int>() : 10;

                Pixels &pixels = holder->getScratchPad();
                pixels.clear();

                holder->getRenderer()->drawText(pixels, text, x, y, font, color,
                                                wrap);

                size_t frameBytes = writeDenseFramebuffer(
                    raw, maxBytes, holder->getWidth(), holder->getHeight(),
                    format, pixels);
                if (frameBytes == 0) {
                    jac::Logger::error(
                        "Renderer.drawText: ArrayBuffer too small or invalid format");
                    return jac::Value::undefined(ctx);
                }

                return jac::Value(ctx, static_cast<int>(frameBytes));
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
    }
};
