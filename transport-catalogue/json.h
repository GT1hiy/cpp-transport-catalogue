#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node final : private std::variant<std::nullptr_t, std::string, int, double, bool, Array, Dict> {
    public:
        using Var = std::variant<std::nullptr_t, std::string, int, double, bool, Array, Dict>;
        using variant::variant;

        Node(const char* value);

        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        const Array& AsArray() const;
        const Dict& AsMap() const;
        int AsInt() const;
        const std::string& AsString() const;
        bool AsBool() const;
        double AsDouble() const;

        const Var& GetVariant() const;

        bool operator==(const Node& rhs) const;
        bool operator!=(const Node& rhs) const;
    };

    class Document {
    public:
        explicit Document(Node root);
        const Node& GetRoot() const;
        bool operator==(const Document& rhs) const;
        bool operator!=(const Document& rhs) const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);
    void Print(const Document& doc, std::ostream& output);

} // namespace json