// Copyright (c) 2020, Nohzmi. All rights reserved.

/**
* @file eventpp.hpp
* @author Nohzmi
* @version 1.0
*/

#pragma once
#include <memory>
#include <tuple>
#include <vector>

/**
* @addtogroup Eventpp
* @{
*/

namespace eventpp
{
    namespace details
    {
        template<typename FuncT>
        struct function_traits
        {
            using type = typename function_traits<decltype(&FuncT::operator())>::type;
        };

        template<typename Return, typename ...Args>
        struct function_traits<Return(Args...)>
        {
            using type = Return(*)(Args...);
        };

        template<typename ClassT, typename Return, typename ...Args>
        struct function_traits<Return(ClassT::*)(Args...)> final : public function_traits<Return(Args...)> {};

        template<typename ClassT, typename Return, typename ...Args>
        struct function_traits<Return(ClassT::*)(Args...) const> final : public function_traits<Return(Args...)> {};

        template<typename FuncT>
        class callback;

        template<typename Return, typename ...Args>
        class callback<Return(*)(Args...)>
        {
            using FuncT = Return(*)(Args...);

        public:

            callback() = default;
            virtual ~callback() = default;
            callback(const callback&) = delete;
            callback(callback&&) noexcept = delete;
            callback& operator=(const callback&) = delete;
            callback& operator=(callback&&) noexcept = delete;

            virtual bool operator==(const callback<Return(*)(Args...)>& rhs) const noexcept = 0;
            virtual Return operator()(Args... args) const noexcept = 0;
            virtual std::unique_ptr<callback<FuncT>> clone() const noexcept = 0;
        };

        template<typename FuncT>
        class free_callback;

        template<typename Return, typename ...Args>
        class free_callback<Return(*)(Args...)> final : public callback<Return(*)(Args...)>
        {
            using FuncT = Return(*)(Args...);

        public:

            free_callback() = delete;
            ~free_callback() final = default;
            free_callback(const free_callback&) = delete;
            free_callback(free_callback&&) noexcept = delete;
            free_callback& operator=(const free_callback&) = delete;
            free_callback& operator=(free_callback&&) noexcept = delete;

            free_callback(FuncT func) noexcept :
                m_func{ func }
            {
            }

            bool operator==(const callback<FuncT>& rhs) const noexcept final
            {
                const auto pc{ const_cast<callback<FuncT>*>(&rhs) };
                const auto fc{ static_cast<free_callback*>(pc) };
                const bool is_func{ fc ? fc->m_func == m_func : false };

                return is_func;
            }

            Return operator()(Args... args) const noexcept final
            {
                return m_func(args...);
            }

            std::unique_ptr<callback<FuncT>> clone() const noexcept final
            {
                return std::make_unique<free_callback>(m_func);
            }

        private:

            FuncT m_func;
        };

        template<typename ClassT, typename FuncT>
        class member_callback;

        template<typename ClassT, typename Return, typename ...Args>
        class member_callback<ClassT, Return(ClassT::*)(Args...)> final : public callback<Return(*)(Args...)>
        {
            using FuncClassT = Return(ClassT::*)(Args...);
            using CallerT = Return(*)(Args...);

        public:

            member_callback() = delete;
            ~member_callback() final = default;
            member_callback(const member_callback&) = delete;
            member_callback(member_callback&&) noexcept = delete;
            member_callback& operator=(const member_callback&) = delete;
            member_callback& operator=(member_callback&&) noexcept = delete;

            member_callback(FuncClassT func, ClassT* callee) noexcept :
                m_func{ func },
                m_callee{ callee }
            {
            }

            bool operator==(const callback<CallerT>& rhs) const noexcept final
            {
                const auto pc{ const_cast<callback<CallerT>*>(&rhs) };
                const auto mc{ static_cast<member_callback*>(pc) };
                const bool is_func{ mc ? mc->m_func == m_func : false };
                const bool is_callee{ mc ? mc->m_callee == m_callee : false };

                return is_func && is_callee;
            }

            Return operator()(Args... args) const noexcept final
            {
                return (m_callee->*m_func)(args...);
            }

            std::unique_ptr<callback<CallerT>> clone() const noexcept final
            {
                return std::make_unique<member_callback>(m_func, m_callee);
            }

        private:

            FuncClassT m_func;
            ClassT* m_callee;
        };

        template<typename LambdaT, typename FuncT = LambdaT>
        class lambda_callback;

        template<typename LambdaT, typename Return, typename ...Args>
        class lambda_callback<LambdaT, Return(*)(Args...)> final : public callback<Return(*)(Args...)>
        {
            using FuncT = Return(*)(Args...);

        public:

            lambda_callback() = delete;
            ~lambda_callback() final = default;
            lambda_callback(const lambda_callback&) = delete;
            lambda_callback(lambda_callback&&) noexcept = delete;
            lambda_callback& operator=(const lambda_callback&) = delete;
            lambda_callback& operator=(lambda_callback&&) noexcept = delete;

            lambda_callback(LambdaT func, size_t hash) noexcept :
                m_func{ std::make_unique<LambdaT>(func) },
                m_hash{ hash }
            {
            }

            bool operator==(const callback<FuncT>& rhs) const noexcept final
            {
                const auto pc{ const_cast<callback<FuncT>*>(&rhs) };
                const auto fc{ static_cast<lambda_callback*>(pc) };
                const bool is_func{ fc ? fc->m_hash == m_hash : false };

                return is_func;
            }

            Return operator()(Args... args) const noexcept final
            {
                return (*m_func)(args...);
            }

            std::unique_ptr<callback<FuncT>> clone() const noexcept final
            {
                return std::make_unique<lambda_callback>(*m_func, m_hash);
            }

        private:

            std::unique_ptr<LambdaT> m_func;
            size_t m_hash;
        };

        template<typename FuncT>
        class free_bind_callback;

        template<typename Return, typename ...Args>
        class free_bind_callback<Return(*)(Args...)> final : public callback<Return(*)()>
        {
            using FuncT = Return(*)(Args...);
            using CallerT = Return(*)();
            using ArgsT = std::tuple<Args...>;

        public:

            free_bind_callback() = delete;
            ~free_bind_callback() final = default;
            free_bind_callback(const free_bind_callback&) = delete;
            free_bind_callback(free_bind_callback&&) noexcept = delete;
            free_bind_callback& operator=(const free_bind_callback&) = delete;
            free_bind_callback& operator=(free_bind_callback&&) noexcept = delete;

            free_bind_callback(FuncT func, ArgsT args) noexcept :
                m_func{ func },
                m_args{ args }
            {
            }

            bool operator==(const callback<CallerT>& rhs) const noexcept final
            {
                const auto pc{ const_cast<callback<CallerT>*>(&rhs) };
                const auto fc{ static_cast<free_bind_callback*>(pc) };
                const bool is_func{ fc ? fc->m_func == m_func : false };
                const bool is_args{ fc ? fc->m_args == m_args : false };

                return is_func && is_args;
            }

            Return operator()() const noexcept final
            {
                return apply(m_func, m_args);
            }

            std::unique_ptr<callback<CallerT>> clone() const noexcept final
            {
                return std::make_unique<free_bind_callback>(m_func, m_args);
            }

        private:

            template<typename FuncT, typename ArgsT>
            Return apply(FuncT func, ArgsT args) const noexcept
            {
                static constexpr auto size = std::tuple_size<ArgsT>::value;
                return apply_impl(func, args, std::make_index_sequence<size>{});
            }

            template<typename FuncT, typename ArgsT, int... idx>
            Return apply_impl(FuncT func, ArgsT args, std::index_sequence<idx...>) const noexcept
            {
                return (func)(std::get<idx>(args)...);
            }

            FuncT m_func;
            ArgsT m_args;
        };

        template<typename ClassT, typename FuncT>
        class member_bind_callback;

        template<typename ClassT, typename Return, typename ...Args>
        class member_bind_callback<ClassT, Return(ClassT::*)(Args...)> final : public callback<Return(*)()>
        {
            using FuncClassT = Return(ClassT::*)(Args...);
            using CallerT = Return(*)();
            using ArgsT = std::tuple<Args...>;

        public:

            member_bind_callback() = delete;
            ~member_bind_callback() final = default;
            member_bind_callback(const member_bind_callback&) = delete;
            member_bind_callback(member_bind_callback&&) noexcept = delete;
            member_bind_callback& operator=(const member_bind_callback&) = delete;
            member_bind_callback& operator=(member_bind_callback&&) noexcept = delete;

            member_bind_callback(FuncClassT func, ClassT* callee, ArgsT args) noexcept :
                m_func{ func },
                m_callee{ callee },
                m_args{ args }
            {
            }

            bool operator==(const callback<CallerT>& rhs) const noexcept final
            {
                const auto pc{ const_cast<callback<CallerT>*>(&rhs) };
                const auto mc{ static_cast<member_bind_callback*>(pc) };
                const bool is_func{ mc ? mc->m_func == m_func : false };
                const bool is_callee{ mc ? mc->m_callee == m_callee : false };
                const bool is_args{ mc ? mc->m_args == m_args : false };

                return is_func && is_callee && is_args;
            }

            Return operator()() const noexcept final
            {
                return apply(m_func, m_callee, m_args);
            }

            std::unique_ptr<callback<CallerT>> clone() const noexcept final
            {
                return std::make_unique<member_bind_callback>(m_func, m_callee, m_args);
            }

        private:

            template<typename FuncClassT, typename Class, typename ArgsT>
            Return apply(FuncClassT func, Class* callee, ArgsT args) const noexcept
            {
                static constexpr auto size = std::tuple_size<ArgsT>::value;
                return apply_impl(func, callee, args, std::make_index_sequence<size>{});
            }

            template<typename FuncClassT, typename Class, typename ArgsT, size_t... idx>
            Return apply_impl(FuncClassT func, Class* callee, ArgsT args, std::index_sequence<idx...>) const noexcept
            {
                return (callee->*func)(std::get<idx>(args)...);
            }

            FuncClassT m_func;
            ClassT* m_callee;
            ArgsT m_args;
        };

        template<typename LambdaT, typename FuncT = LambdaT>
        class lambda_bind_callback;

        template<typename LambdaT, typename Return, typename ...Args>
        class lambda_bind_callback<LambdaT, Return(*)(Args...)> final : public callback<Return(*)()>
        {
            using FuncT = Return(*)();
            using ArgsT = std::tuple<Args...>;

        public:

            lambda_bind_callback() = delete;
            ~lambda_bind_callback() final = default;
            lambda_bind_callback(const lambda_bind_callback&) = delete;
            lambda_bind_callback(lambda_bind_callback&&) noexcept = delete;
            lambda_bind_callback& operator=(const lambda_bind_callback&) = delete;
            lambda_bind_callback& operator=(lambda_bind_callback&&) noexcept = delete;

            lambda_bind_callback(LambdaT func, size_t hash, ArgsT args) noexcept :
                m_func{ std::make_unique<LambdaT>(func) },
                m_hash{ hash },
                m_args{ args }
            {
            }

            bool operator==(const callback<FuncT>& rhs) const noexcept final
            {
                const auto pc{ const_cast<callback<FuncT>*>(&rhs) };
                const auto fc{ static_cast<lambda_bind_callback*>(pc) };
                const bool is_func{ fc ? fc->m_hash == m_hash : false };
                const bool is_args{ fc ? fc->m_args == m_args : false };

                return is_func && is_args;
            }

            Return operator()() const noexcept final
            {
                return apply(*m_func, m_args);
            }

            std::unique_ptr<callback<FuncT>> clone() const noexcept final
            {
                return std::make_unique<lambda_bind_callback>(*m_func, m_hash, m_args);
            }

        private:

            template<typename FuncT, typename ArgsT>
            Return apply(FuncT func, ArgsT args) const noexcept
            {
                static constexpr auto size = std::tuple_size<ArgsT>::value;
                return apply_impl(func, args, std::make_index_sequence<size>{});
            }

            template<typename FuncT, typename ArgsT, size_t... idx>
            Return apply_impl(FuncT func, ArgsT args, std::index_sequence<idx...>) const noexcept
            {
                return (func)(std::get<idx>(args)...);
            }

            std::unique_ptr<LambdaT> m_func;
            size_t m_hash;
            ArgsT m_args;
        };
    }

    template<typename FuncT>
    class delegate;

    /**
    * Store a function
    */
    template<typename Return, typename ...Args>
    class delegate<Return(*)(Args...)> final
    {
        using FuncT = Return(*)(Args...);
        using CallbackT = std::unique_ptr<details::callback<FuncT>>;

    public:

        delegate() = default;
        ~delegate() = default;
        delegate(delegate&&) noexcept = default;
        delegate& operator=(delegate&&) noexcept = default;

        delegate(const delegate& copy)
        {
            if (copy.m_callback)
                m_callback = copy.m_callback->clone();
        }

        delegate& operator=(const delegate& copy)
        {
            if (copy.m_callback)
                m_callback = copy.m_callback->copy();

            return this;
        }

        /**
        * Replace stored function \n
        * See make_callback() and make_lambda()
        * @param callback
        */
        delegate& operator=(CallbackT callback) noexcept
        {
            m_callback = std::forward<CallbackT>(callback);
            return *this;
        }

        /**
        * Call stored function
        * @param args
        */
        Return operator()(Args... args) const noexcept
        {
            return m_callback ? (*m_callback)(args...) : Return();
        }

        /**
        * Returns whether or not it store a function
        */
        operator bool() const noexcept
        {
            return static_cast<bool>(m_callback);
        }

        /**
        * Clear stored functions
        */
        void clear() noexcept
        {
            m_callback.reset(nullptr);
        }

    private:

        CallbackT m_callback;
    };

    template<typename FuncT>
    class event;

    /**
    * Store multiple functions
    */
    template<typename Return, typename ...Args>
    class event<Return(*)(Args...)> final
    {
        using FuncT = Return(*)(Args...);
        using CallbackT = std::unique_ptr<details::callback<FuncT>>;

    public:

        event() = default;
        ~event() = default;
        event(event&&) noexcept = default;
        event& operator=(event&&) noexcept = default;

        event(const event& copy)
        {
            m_callbacks.clear();

            for (const CallbackT& callback : copy.m_callbacks)
                m_callbacks.emplace_back(callback->clone());
        }

        event& operator=(const event& copy)
        {
            m_callbacks.clear();

            for (const CallbackT& callback : copy.m_callbacks)
                m_callbacks.emplace_back(callback->clone());

            return this;
        }

        /**
        * Subscribe a function \n
        * See make_callback() and make_lambda()
        * @param callback
        */
        void operator+=(CallbackT callback) noexcept
        {
            for (auto i{ 0u }; i < m_callbacks.size(); ++i)
                if (*callback == *m_callbacks[i])
                    return;

            m_callbacks.emplace_back(std::forward<CallbackT>(callback));
        }

        /**
        * Unsubscribe a function \n
        * See make_callback() and make_lambda()
        * @param callback
        */
        void operator-=(CallbackT callback) noexcept
        {
            for (auto i{ 0u }; i < m_callbacks.size(); ++i)
            {
                if (*callback == *m_callbacks[i])
                {
                    m_callbacks.erase(m_callbacks.begin() + i);
                    return;
                }
            }
        }

        /**
        * Clear all subscribed functions
        */
        void clear()
        {
            m_callbacks.clear();
        }

        /**
        * Call all subscribed functions
        * @param args
        */
        void invoke(Args... args) const noexcept
        {
            for (auto i{ 0u }; i < m_callbacks.size(); ++i)
                (*m_callbacks[i])(std::forward<Args>(args)...);
        }

        /**
        * Returns whether or not the event stores functions
        */
        bool empty() noexcept
        {
            return m_callbacks.size() == 0;
        }

    private:

        std::vector<CallbackT> m_callbacks;
    };

    /**
    * Returns a callback on a free function
    * @param func
    */
    template<typename FuncT>
    std::unique_ptr<details::free_callback<FuncT>> make_callback(FuncT&& func) noexcept
    {
        return std::make_unique<details::free_callback<FuncT>>(func);
    }

    /**
    * Returns a callback on a free function \n
    * Allow to bind arguments with the function
    * @param func
    * @param args
    */
    template<typename FuncT, typename ...Args>
    std::unique_ptr<details::free_bind_callback<FuncT>> make_callback(FuncT&& func, Args&&... args) noexcept
    {
        return std::make_unique<details::free_bind_callback<FuncT>>(func, std::make_tuple(args...));
    }

    /**
    * Returns a callback on a member function
    * @param func
    * @param obj
    */
    template<typename ClassT, typename FuncT>
    std::unique_ptr<details::member_callback<ClassT, FuncT>> make_callback(FuncT&& func, ClassT*&& obj) noexcept
    {
        return std::make_unique<details::member_callback<ClassT, FuncT>>(func, obj);
    }

    /**
    * Returns a callback on a member function \n
    * Allow to bind arguments with the function
    * @param func
    * @param obj
    * @param args
    */
    template<typename ClassT, typename FuncT, typename ...Args>
    std::unique_ptr<details::member_bind_callback<ClassT, FuncT>> make_callback(FuncT&& func, ClassT*&& obj, Args&&... args) noexcept
    {
        return std::make_unique<details::member_bind_callback<ClassT, FuncT>>(func, obj, std::make_tuple(args...));
    }

    /**
    * Returns a callback on a lambda function
    * @param func
    */
    template<typename LambdaT, typename FuncT = typename details::function_traits<LambdaT>::type>
    std::unique_ptr<details::lambda_callback<LambdaT, FuncT>> make_lambda(LambdaT func) noexcept
    {
        return std::make_unique<details::lambda_callback<LambdaT, FuncT>>(func, typeid(func).hash_code());
    }

    /**
    * Returns a callback on a lambda function \n
    * Allow to bind arguments with the function
    * @param func
    * @param args
    */
    template<typename LambdaT, typename FuncT = typename details::function_traits<LambdaT>::type, typename ...Args>
    std::unique_ptr<details::lambda_bind_callback<LambdaT, FuncT>> make_lambda(LambdaT func, Args&&... args) noexcept
    {
        return std::make_unique<details::lambda_bind_callback<LambdaT, FuncT>>(func, typeid(func).hash_code(), std::make_tuple(args...));
    }
}

/**
* @}
*/

