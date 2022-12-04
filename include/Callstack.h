#pragma once

template <typename Key, typename Value = unsigned char>
class Callstack {
public:
    class Iterator;
 
    class Context {
    public:
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        explicit Context(Key* k)
            : key_(k), next_(Callstack<Key, Value>::top_) {
            val_ = reinterpret_cast<unsigned char*>(this);
            Callstack<Key, Value>::top_ = this;
        }
 
        Context(Key* k, Value& v)
            : key_(k), val_(&v), next_(Callstack<Key, Value>::top_) {
            Callstack<Key, Value>::top_ = this;
        }
 
        ~Context() {
            Callstack<Key, Value>::top_ = next_;
        }
 
        Key* getKey() {
            return key_;
        }
 
        Value* getValue() {
            return val_;
        }
 
    private:
        friend class Callstack<Key, Value>;
        friend class Callstack<Key, Value>::Iterator;
        Key* key_;
        Value* val_;
        Context* next_;
    };
 
    class Iterator {
    public:
        Iterator(Context* ctx) : ctx_(ctx) {}
        Iterator& operator++() {
            if (ctx_)
                ctx_ = ctx_->next_;
            return *this;
        }
 
        bool operator!=(const Iterator& other) {
            return ctx_ != other.ctx_;
        }
 
        Context* operator*() {
            return ctx_;
        }
 
    private:
        Context* ctx_;
    };
 
    // Determine if the specified owner is on the stack
    // \return
    //  The address of the value if present, nullptr if not present
    static Value* contains(const Key* k) {
        Context* elem = top_;
        while (elem) {
            if (elem->key_ == k)
                return elem->val_;
            elem = elem->next_;
        }
        return nullptr;
    }
 
    static Iterator begin() {
        return Iterator(top_);
    }
 
    static Iterator end() {
        return Iterator(nullptr);
    }
 
private:
    inline static thread_local Context* top_ = nullptr;
};