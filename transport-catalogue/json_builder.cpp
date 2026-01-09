#include <variant>
#include <optional>
#include "json_builder.h"

namespace json {

Builder::Builder() {
    root_ = Node(); // Создаем корневой узел
    nodes_stack_.push_back(&root_); // Добавляем корневой узел в стек
}

Node::Var& Builder::GetCurrentValue() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("No current value in the stack");
    }
    return nodes_stack_.back()->GetValue();
}

void Builder::AddNode(Node&& node, bool one_shot) {
    Node::Var& host_value = GetCurrentValue();

    if (std::holds_alternative<std::nullptr_t>(host_value)) {
        // Заменяем корневой узел nullptr на новое значение
        host_value = std::move(node.GetValue());
        // Если это контейнер (не one_shot), добавляем его в стек для дальнейшего построения
        if (!one_shot) {
            nodes_stack_.push_back(nodes_stack_.back());
        }
    } else if (auto* dict = std::get_if<Dict>(&host_value)) {
        // Для словаря используем сохраненный ключ
        if (!current_key_) {
            throw std::logic_error("No key provided for dictionary value");
        }
        // Вставляем и получаем итератор на вставленный элемент
        auto [it, inserted] = dict->emplace(std::move(*current_key_), std::move(node));
        current_key_.reset(); // Ключ использован, сбрасываем

        if (!one_shot) {
            // Добавляем указатель на только что вставленный узел
            nodes_stack_.push_back(&it->second);
        }

    } else if (auto* array = std::get_if<Array>(&host_value)) {
        // Для массива просто добавляем узел
        array->push_back(std::move(node));

        if (!one_shot) {
            // Получаем неконстантную ссылку на последний элемент
            nodes_stack_.push_back(&array->back());
        }
    } else {
        throw std::logic_error("AddNode called outside dictionary or array");
    }
}

DictItemContext Builder::StartDict() {
    Node::Var& host_value = GetCurrentValue();

    // Проверяем, можно ли начать словарь в текущем контексте
    // Если current_key_ имеет значение, значит мы в процессе заполнения ключа в словаре
    if (std::holds_alternative<Dict>(host_value) && !current_key_) {
        throw std::logic_error("StartDict called in dictionary without a key");
    }

    if (!std::holds_alternative<std::nullptr_t>(host_value) &&
        !std::holds_alternative<Dict>(host_value) &&
        !std::holds_alternative<Array>(host_value)) {
        throw std::logic_error("StartDict called in invalid context");
    }

    // Создаем новый словарь как Node
    Node dict_node{Dict{}};
    AddNode(std::move(dict_node), false);

    return DictItemContext(*this);
}

DictKeyContext Builder::Key(std::string key) {
    Node::Var& host_value = GetCurrentValue();

    if (!std::holds_alternative<Dict>(host_value)) {
        throw std::logic_error("Key method called outside a dictionary");
    }

    // Если current_key_ уже имеет значение, значит предыдущий ключ не получил значение
    if (current_key_) {
        throw std::logic_error("Key called immediately after another Key without Value");
    }

    current_key_ = std::move(key); // Устанавливаем ключ
    return DictKeyContext(*this);
}

Builder& Builder::Value(Node::Var value) {
    Node::Var& host_value = GetCurrentValue();

    // Проверяем, можно ли добавить значение в текущем контексте
    // Если в словаре и нет установленного ключа - ошибка
    if (std::holds_alternative<Dict>(host_value) && !current_key_) {
        throw std::logic_error("Value called in dictionary without a key");
    }

    // Проверяем, что не пытаемся добавить значение в неподдерживаемый контекст
    if (!std::holds_alternative<std::nullptr_t>(host_value) &&
        !std::holds_alternative<Dict>(host_value) &&
        !std::holds_alternative<Array>(host_value)) {
        throw std::logic_error("Value called in invalid context");
    }

    // Используем конструктор Node от Node::Var
    Node node(std::move(value));
    AddNode(std::move(node), true);

    return *this;
}

Builder& Builder::EndDict() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("No dictionary to end");
    }

    Node::Var& host_value = GetCurrentValue();
    if (!std::holds_alternative<Dict>(host_value)) {
        throw std::logic_error("EndDict called when current value is not a dictionary");
    }

    nodes_stack_.pop_back();
    return *this;
}

ArrayItemContext Builder::StartArray() {
    Node::Var& host_value = GetCurrentValue();

    // Проверяем, можно ли начать массив в текущем контексте
    // Если в словаре и нет установленного ключа - ошибка
    if (std::holds_alternative<Dict>(host_value) && !current_key_)  {
        throw std::logic_error("StartArray called in dictionary without a key");
    }

    if (!std::holds_alternative<std::nullptr_t>(host_value) &&
        !std::holds_alternative<Dict>(host_value) &&
        !std::holds_alternative<Array>(host_value)) {
        throw std::logic_error("StartArray called in invalid context");
    }

    // Создаем новый массив как Node
    Node array_node{Array{}};
    AddNode(std::move(array_node), false);

    return ArrayItemContext(*this);
}

Builder& Builder::EndArray() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("No array to end");
    }

    Node::Var& host_value = GetCurrentValue();
    if (!std::holds_alternative<Array>(host_value)) {
        throw std::logic_error("EndArray called when current value is not an array");
    }

    nodes_stack_.pop_back();
    return *this;
}

Node Builder::Build() {
    // Проверяем, что стек содержит только корневой узел
    if (nodes_stack_.size() != 1) {
        throw std::logic_error("Build called with unfinished structures");
    }

    // Проверяем, что все структуры закрыты (нет незавершенных массивов/словарей)
    Node::Var& root_value = GetCurrentValue();
    if (std::holds_alternative<std::nullptr_t>(root_value)) {
        throw std::logic_error("Build called on empty builder");
    }

    // Проверяем, что нет незакрытых ключей
    if (current_key_) {
        throw std::logic_error("Build called with unfinished key");
    }

    // Возвращаем корневой узел
    return std::move(root_);
}

// Реализации методов контекстных классов ===========================
DictKeyContext DictItemContext::Key(std::string key) {
    builder_.Key(std::move(key));
    return DictKeyContext(builder_);
}

Builder& DictItemContext::EndDict() {
    return builder_.EndDict();
}

DictItemContext DictKeyContext::Value(Node::Var value) {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

DictItemContext DictKeyContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext DictKeyContext::StartArray() {
    return builder_.StartArray();
}

ArrayItemContext ArrayItemContext::Value(Node::Var value) {
    builder_.Value(std::move(value));
    return *this;
}

Builder& ArrayItemContext::EndArray() {
    return builder_.EndArray();
}

DictItemContext ArrayItemContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext ArrayItemContext::StartArray() {
    return builder_.StartArray();
}

} // namespace json
