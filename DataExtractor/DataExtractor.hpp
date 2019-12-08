#ifndef DATAEXTRACTOR_DATAEXTRACTOR_HPP
#define DATAEXTRACTOR_DATAEXTRACTOR_HPP

#include <variant>
#include <type_traits>
#include <tuple>
#include <utility>

template <typename ReasoningType_, typename ValueType_>
using MaybeValue = std::variant<ReasoningType_, ValueType_>;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template<class T> struct always_false : std::false_type {};
namespace detail
{
    template <typename... Args>
    struct type_list {};

    template<class ReturnType_, class ReasoningType_, class... Args_>
    struct ConcreteFunctionExtractor
    {
        using FunctionType = MaybeValue<ReasoningType_, ReturnType_>(*)(Args_...);
        using Arguments = type_list<Args_...>;
        using ReturnType = ReturnType_;
        using SavedType = typename std::remove_pointer_t<ReturnType_>;
        using ReasoningType = ReasoningType_;

        ConcreteFunctionExtractor(FunctionType extractor) : extractor_(extractor) {}

        template <class GetableContainer>
        MaybeValue<ReasoningType_, ReturnType_> extract(const GetableContainer& container) const
        {
            return extractor_(container.template get<typename std::decay_t<Args_>>()...);
        }
    private:
        FunctionType extractor_ ;
    };


    template<class ReturnType_, class ReasoningType_, class Extractor, class... Args_>
    struct ConcreteExtractor;

    template<class ReturnType_, class ReasoningType_, class Extractor, class... Args_>
    struct ConcreteExtractor<ReturnType_, ReasoningType_, Extractor, type_list<Args_...>>
    {
        using Arguments = type_list<Args_...>;
        using ReturnType = ReturnType_;
        using SavedType = typename std::remove_pointer_t<ReturnType_>;
        using ReasoningType = ReasoningType_;

        ConcreteExtractor(Extractor extractor) : extractor_{std::move(extractor)}{}

        template <class GetableContainer>
        MaybeValue<ReasoningType_, ReturnType_> extract(const GetableContainer& container) const
        {
            return extractor_(container.template get<typename std::decay_t<Args_>>()...);
        }
    private:
        Extractor extractor_;
    };

    template<class ReasoningType_, class InputType_, class ExtractorType_, class... PrevExtractorsInChain_>
    struct Extractable : public Extractable<ReasoningType_, InputType_, PrevExtractorsInChain_...>
    {
        using Base = Extractable<ReasoningType_, InputType_, PrevExtractorsInChain_...>;
        using SavedType = typename ExtractorType_::SavedType;
        using ExtractorReturnType = typename ExtractorType_::ReturnType;
        using ReasoningType = ReasoningType_;

        Extractable(const InputType_& input,
                    const ExtractorType_& extractor,
                    PrevExtractorsInChain_... other) : Base(input, other...)
        {
            if (!Base::reasoning)
            {
                std::visit([&](auto&& arg){
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::decay_t<ExtractorReturnType>>)
                        saved_ = arg;
                    else if constexpr  (std::is_same_v<T, std::decay_t<ReasoningType_>>)
                        Base::reasoning = arg;
                    else
                        static_assert(always_false<T>::value, "something went wrong when creating visitor");
                }, extractor.extract(static_cast<const Base&>(*this)));
            }
        }

        template<class T>
        const T& get() const
        {
            if constexpr (std::is_same_v<std::decay_t<T>, SavedType>)
                return *saved_;
            else
                return this->Base::template get<T>();
        }

    private:
        SavedType* saved_;
    };

    template<class ReasoningType_, class InputType_>
    struct InputAndReasoning
    {
        explicit InputAndReasoning(const InputType_& input) : input_(input){}

        template<class T>
        const T& get() const
        {
            if constexpr (std::is_same_v<std::decay_t<T>, InputType_>)
                return input_;
            else
                static_assert(always_false<T>::value, "Extractable does not contain this type");
        }
        std::optional<ReasoningType_> reasoning{};
    private:
        const InputType_& input_;
    };

    template<class ReasoningType_, class InputType_, class ExtractorType_>
    struct Extractable<ReasoningType_, InputType_, ExtractorType_> : public InputAndReasoning<ReasoningType_, InputType_>
    {
        using Base = InputAndReasoning<ReasoningType_, InputType_>;
        using SavedType = typename ExtractorType_::SavedType;
        using ExtractorReturnType = typename ExtractorType_::ReturnType;
        using ReasoningType = ReasoningType_;
        Extractable(const InputType_& input,
                    const ExtractorType_& extractor)
                : Base(input)
        {
            std::visit([&](auto&& arg){
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::decay_t<ExtractorReturnType>>)
                    saved_ = arg;
                else if constexpr  (std::is_same_v<T, std::decay_t<ReasoningType_>>)
                    Base::reasoning = arg;
                else
                    static_assert(always_false<T>::value, "something went wrong when creating visitor");
            }, extractor.extract(static_cast<const Base&>(*this)));
        }

        template<class T>
        const T& get() const
        {
            if constexpr (std::is_same_v<std::decay_t<T>, SavedType>)
                return *saved_;
            else
                return this->Base::template get<T>();
        }
    private:
        SavedType* saved_;
    };

    template<class T>
    struct ExtractorDataTypes;

    template<class ClassType_, class ReturnType_, class ReasoningType_, class... Args_>
    struct ExtractorDataTypes<MaybeValue<ReasoningType_, ReturnType_>(ClassType_::*)(Args_...) const>
    {
        using FunctionType = MaybeValue<ReasoningType_, ReturnType_>(ClassType_::*)(Args_...);
        using Arguments = type_list<Args_...>;
        using ReturnType = ReturnType_;
        using ReasoningType = ReasoningType_;
    };
    
    template<class T>
    struct BuilderDataTypes;

    template<class ClassType_, class ReturnType_, class... Args_>
    struct BuilderDataTypes<ReturnType_(ClassType_::*)(Args_...) const>
    {
        using Arguments = type_list<Args_...>;
        using ReturnType = ReturnType_;
    };


    template<class ReturnType_, class Args>
    struct BuilderRunner;

    template<class ReturnType_, class... BuilderArgs>
    struct BuilderRunner<ReturnType_, type_list<BuilderArgs...>>
    {
        template<class ExtractableType_, class BuilderType_>
        static ReturnType_ run(ExtractableType_ extractable, BuilderType_ builder)
        {
            return builder(extractable.template get<std::decay_t<BuilderArgs>>()...);
        }
    };

    template <class Extractor_, class... Extractors_>
    struct DataExtractor
    {
        using ReasoningType = typename Extractor_::ReasoningType;
        explicit DataExtractor(Extractor_ ex, Extractors_... exs)
            : concreteExtractors_{std::move(ex), std::move(exs)...}
        {
        }

        template <class InputType_>
        Extractable<ReasoningType, InputType_, Extractor_, Extractors_...> makeExtractable (InputType_ input) const
        {
            return {input, std::get<Extractor_>(concreteExtractors_), std::get<Extractors_>(concreteExtractors_)...};
        }

        template <class InputType_, class Builder_>
        MaybeValue<
                ReasoningType,
                typename BuilderDataTypes<decltype(&Builder_::operator())>::ReturnType>
        extract(const InputType_& input, Builder_ builder) const
        {
            auto extractable = makeExtractable(input);
            if (extractable.reasoning) {
                return *extractable.reasoning;
            }
            else {
                using ReturnType = typename BuilderDataTypes<decltype(&Builder_::operator())>::ReturnType;
                using ArgumentsTypeList = typename BuilderDataTypes<decltype(&Builder_::operator())>::Arguments;
                return BuilderRunner<ReturnType, ArgumentsTypeList>::run(extractable, std::forward<Builder_>(builder));
            }
        }

        template<class InputType_, class BuilderReturnType_, class... BuilderArgs_>
        MaybeValue<
                ReasoningType,
                BuilderReturnType_>
        extract(const InputType_& input, BuilderReturnType_(builderFun)(BuilderArgs_...)) const
        {
            auto extractable = makeExtractable(input);
            if (extractable.reasoning)
                return *extractable.reasoning;
            else
                return builderFun(extractable.template get<BuilderArgs_>()...);
        }

    private:
        std::tuple<Extractor_, Extractors_...> concreteExtractors_;
    };

    template <class... Extractors>
    struct BuildWithReversedExtractors;

    template <>
    struct BuildWithReversedExtractors<>
    {
        template<class... Extractors_>
        static detail::DataExtractor<Extractors_...> build(Extractors_&&... extractors)
        {
            return detail::DataExtractor<Extractors_...>{std::forward<Extractors_>(extractors)...};
        }
    };


    template <class Extractor, class... ExtractorsPrev_>
    struct BuildWithReversedExtractors<Extractor, ExtractorsPrev_...>
    {
        template <class... ExtractorsNext_>
        static auto build(
                Extractor&& extractor,
                ExtractorsPrev_&&... extractorsPrev,
                ExtractorsNext_&&... extractorsNext)
        {
            return BuildWithReversedExtractors<ExtractorsPrev_&&...>
                    ::build(std::forward<ExtractorsPrev_>(extractorsPrev)...,
                            std::forward<Extractor>(extractor),
                            std::forward<ExtractorsNext_>(extractorsNext)...);
        }
    };

    template <class... ConcreteExtractors_>
    auto makeDataExtractorFromConcreteExtractors(ConcreteExtractors_&&... extractors) -> decltype(auto)
    {
        return BuildWithReversedExtractors<ConcreteExtractors_&&...>
                ::build(std::forward<ConcreteExtractors_>(extractors)...);
    }
} // namespace detail



template <class ReturnType_, class ReasoningType_, class... Args_>
detail::ConcreteFunctionExtractor<
        ReturnType_,
        ReasoningType_,
        Args_...>
makeExtractor(MaybeValue<ReasoningType_, ReturnType_>(inputFunction)(Args_...))
{
    return {inputFunction};
}


template <class ExtractorType_>
detail::ConcreteExtractor<
        typename detail::ExtractorDataTypes<decltype(&std::decay_t<ExtractorType_>::operator())>::ReturnType ,
        typename detail::ExtractorDataTypes<decltype(&std::decay_t<ExtractorType_>::operator())>::ReasoningType ,
        std::decay_t<ExtractorType_>,
        typename detail::ExtractorDataTypes<decltype(&std::decay_t<ExtractorType_>::operator())>::Arguments>
makeExtractor(ExtractorType_&& extractor)
{
    return {std::forward<ExtractorType_>(extractor)};
}


template <class InputType_, class Extractor_, class... Extractors_>
detail::Extractable<typename Extractor_::ReasoningType, InputType_, Extractor_, Extractors_...>
makeExtractableDetailed(InputType_ input, Extractor_ anyExtractor, Extractors_... extractors)
{
    return {input, anyExtractor, extractors...};
}


template<class... Extractors>
struct ReversedExtractableBuilder;

template<>
struct ReversedExtractableBuilder<>
{
    template <class InputType_, class... ExtractorsReversed_>
    static auto build(const InputType_& input, ExtractorsReversed_... extractors)
    {
        return makeExtractableDetailed(input, extractors...);
    }
};


template<class Extractor_, class... ExtractorsPrev_>
struct ReversedExtractableBuilder<Extractor_, ExtractorsPrev_...>
{
    template <class InputType_, class... ExtractorsNext_>
    static auto build(
            const InputType_& input,
            Extractor_ extractor,
            ExtractorsPrev_... extractorsPrev,
            ExtractorsNext_... extractorsNext)
    {
        return ReversedExtractableBuilder<ExtractorsPrev_...>::build(input, extractorsPrev..., extractor, extractorsNext...);
    }
};

template <class InputType_, class... ConcreteExtractors_>
auto makeExtractableReversed(InputType_ input, ConcreteExtractors_... extractors) -> decltype(auto)
{
    return ReversedExtractableBuilder<ConcreteExtractors_...>::build(input, extractors...);
}

template <class InputType_, class... Extractors_>
auto makeExtractableRev(InputType_ input, Extractors_&&... extractors) -> decltype(auto)
{
    return makeExtractableReversed(input, makeExtractor(std::forward<Extractors_>(extractors))...);
}


template <class InputType_, class... Extractors_>
auto makeExtractable(InputType_ input, Extractors_... extractors) -> decltype(auto)
{
    return makeExtractableDetailed(input, makeExtractor(std::forward<Extractors_>(extractors))...);
}

template <class... Extractors_>
auto makeDataExtractor(Extractors_&&... extractors) -> decltype(auto)
{
    return detail::makeDataExtractorFromConcreteExtractors(makeExtractor(std::forward<Extractors_>(extractors))...);
}


#endif //DATAEXTRACTOR_DATAEXTRACTOR_HPP
