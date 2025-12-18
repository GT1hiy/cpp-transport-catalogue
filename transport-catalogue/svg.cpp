#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

    struct Rgb {
        Rgb(): red(0), green(0), blue(0) {}
        Rgb(uint8_t r, uint8_t g, uint8_t b): red(r), green(g), blue(b) {}
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    struct Rgba: public Rgb {
        Rgba(): Rgb(), opacity(1.0) {}
        Rgba(uint8_t r, uint8_t g, uint8_t b, double a): Rgb(r,g,b), opacity(a) {}
        double opacity;
    };

    using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;
    inline const Color NoneColor{ std::monostate() };

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap);
    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join);
    std::ostream& operator<<(std::ostream& out, const Color& color);

    template<class... Ts>
    struct overloads : Ts... { using Ts::operator()...; };

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeWidth(double width) {
            stroke_width_ = width;
            return AsOwner();
        }
        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            line_cap_ = line_cap;
            return AsOwner();
        }
        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            line_join_ = line_join;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            auto visitor = overloads{
                    [&out](std::monostate){ out << "none"; },
                    [&out](const std::string color){ out << color; },
                    [&out](const svg::Rgb rgb){ out << "rgb(" << static_cast<int>(rgb.red) << ","
                                                    << static_cast<int>(rgb.green) << "," << static_cast<int>(rgb.blue) << ")"; },
                    [&out](const svg::Rgba rgba){ out << "rgba(" << static_cast<int>(rgba.red) << ","
                                                      << static_cast<int>(rgba.green) << "," << static_cast<int>(rgba.blue)
                                                      << "," << rgba.opacity << ")"; }
            };

            if (fill_color_) {
                out << " fill=\""sv;
                std::visit(visitor, *fill_color_);
                out << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv;
                std::visit(visitor, *stroke_color_);
                out << "\""sv;
            }
            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (line_cap_) {
                out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
            }
            if (line_join_) {
                out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> line_cap_;
        std::optional<StrokeLineJoin> line_join_;
    };

    struct Point {
        Point() = default;
        Point(double x, double y): x(x), y(y) {}
        double x = 0;
        double y = 0;
    };

    struct RenderContext {
        RenderContext(std::ostream& out): out(out) {}
        RenderContext(std::ostream& out, int indent_step, int indent = 0)
                : out(out), indent_step(indent_step), indent(indent) {}

        RenderContext Indented() const {
            return {out, indent_step, indent + indent_step};
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    class Object {
    public:
        virtual void Render(const RenderContext& context) const;
        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    class ObjectContainer {
    public:
        virtual ~ObjectContainer() = default;
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

        template <typename ObjectType>
        void Add(ObjectType object) {
            AddPtr(std::make_unique<ObjectType>(std::move(object)));
        }
    };

    class Drawable {
    public:
        virtual ~Drawable() = default;
        virtual void Draw(ObjectContainer& container) const = 0;
    };

    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    class Polyline : public Object, public PathProps<Polyline> {
    public:
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_;
    };

    class Text : public Object, public PathProps<Text> {
    public:
        Text() = default;
        Text& SetPosition(Point pos);
        Text& SetOffset(Point offset);
        Text& SetFontSize(uint32_t size);
        Text& SetFontFamily(std::string font_family);
        Text& SetFontWeight(std::string font_weight);
        Text& SetData(std::string data);

        Point GetPosition() const;
        Point GetOffset() const;
        uint32_t GetFontSize() const;
        std::string GetFontFamily() const;
        std::string GetFontWeight() const;
        std::string GetData() const;

        template <typename AttrType>
        inline void RenderAttr(std::ostream& out, std::string_view name, const AttrType& value) const {
            using namespace std::literals;
            out << name << "=\""sv;
            RenderValue(out, value);
            out.put('"');
        }

    private:
        template <typename T>
        void RenderValue(std::ostream& out, const T& value) const {
            out << value;
        }

        void RenderValue(std::ostream& out, const std::string& value) const;
        void HtmlEncodeString(std::ostream& out, std::string sv) const;
        void RenderObject(const RenderContext& context) const override;

        std::string data_;
        std::string font_weight_ = {};
        std::string font_family_ = {};
        uint32_t font_size_ = 1;
        Point position_;
        Point offset_;
    };

    class Document : public Object, public ObjectContainer {
    public:
        using Object::Render;
        void AddPtr(std::unique_ptr<Object>&& obj) override;
        void Render(std::ostream& out) const;

    private:
        void RenderObject(const RenderContext& context) const override;
        std::vector<std::unique_ptr<Object>> objects_;
    };

} // namespace svg