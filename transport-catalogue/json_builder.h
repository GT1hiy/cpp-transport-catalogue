#pragma once

#include "json.h"
#include <optional>

namespace json {

class Builder;
class DictKeyContext;
class ArrayItemContext;

class BaseContext {
public:
    BaseContext(Builder& builder) : builder_(builder) {}

protected:
    Builder& builder_;
};

class DictItemContext : public BaseContext {
public:
    DictItemContext(Builder& builder) : BaseContext(builder) {}

    DictKeyContext Key(std::string key);
    Builder& EndDict();
};

class DictKeyContext : public BaseContext {
public:
    DictKeyContext(Builder& builder) : BaseContext(builder) {}

    DictItemContext Value(Node::Var value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
};

class ArrayItemContext : public BaseContext {
public:
    ArrayItemContext(Builder& builder) : BaseContext(builder) {}

    ArrayItemContext Value(Node::Var value);
    Builder& EndArray();

    // Методы для вложенных структур в массиве
    DictItemContext StartDict();
    ArrayItemContext StartArray();
};

class Builder {
public:
    Builder();
    DictKeyContext Key(std::string key);
    Builder& Value(Node::Var value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    Node Build();

private:
    void AddNode(Node&& node, bool one_shot);
    Node::Var& GetCurrentValue();

    Node root_;
    std::vector<Node*> nodes_stack_;
    std::optional<std::string> current_key_; // Объединяем current_key_ и key_expected_
    // key_expected_ заменяем проверкой current_key_.has_value()

    // Дружественные классы для доступа к приватным методам
    friend class DictKeyContext;
    friend class DictItemContext;
    friend class ArrayItemContext;
};

} // namespace json
