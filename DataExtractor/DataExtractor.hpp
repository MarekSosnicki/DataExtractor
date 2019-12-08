#ifndef DATAEXTRACTOR_DATAEXTRACTOR_HPP
#define DATAEXTRACTOR_DATAEXTRACTOR_HPP

#include <variant>
#include <type_traits>
#include <tuple>
#include <utility>

template <typename ReasoningType_, typename ValueType_>
using MaybeValue = std::variant<ReasoningType_, ValueType_>;

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

        FunctionType extractor_ ;

        template <class GetableContainer>
        MaybeValue<ReasoningType_, ReturnType_> extract(const GetableContainer& container) const
        {
            return extractor_(container.template get<typename std::decay_t<Args_>>()...);
        }
    };


    template<class ReturnType_, class ReasoningType_, class Extractor, class... Args_>
    struct ConcreteExtractor
    {
        using Arguments = type_list<Args_...>;
        using ReturnType = ReturnType_;
        using SavedType = typename std::remove_pointer_t<ReturnType_>;
        using ReasoningType = ReasoningType_;

        Extractor extractor_;

        template <class GetableContainer>
        MaybeValue<ReasoningType_, ReturnType_> extract(const GetableContainer& container) const
        {
            return extractor_(container.template get<typename std::decay_t<Args_>>()...);
        }
    };

    template<class ReturnType_, class ReasoningType_, class Extractor, class... Args_>
    struct ConcreteExtractor<ReturnType_, ReasoningType_, Extractor, type_list<Args_...>>
    {
        using Arguments = type_list<Args_...>;
        using ReturnType = ReturnType_;
        using SavedType = typename std::remove_pointer_t<ReturnType_>;
        using ReasoningType = ReasoningType_;
        Extractor extractor_;
        template <class GetableContainer>
        MaybeValue<ReasoningType_, ReturnType_> extract(const GetableContainer& container) const
        {
            return extractor_(container.template get<typename std::decay_t<Args_>>()...);
        }
    };

    template<class ReasoningType_, class InputType, class ConcreteDataExtractor, class... PrevInChain>
    struct Extractable : public Extractable<ReasoningType_, InputType, PrevInChain...>
    {
        using Base = Extractable<ReasoningType_, InputType, PrevInChain...>;
        using SavedType = typename ConcreteDataExtractor::SavedType;
        using ReasoningType = ReasoningType_;
        using OtherArguments = typename ConcreteDataExtractor::Arguments;
        Extractable(const InputType& input,
                    const ConcreteDataExtractor& extractor,
                    PrevInChain... other) : Base(input, other...)
        {
            if (!Base::reasoning)
            {
                auto result = extractor.extract(static_cast<const Base&>(*this));
                if (auto saved = std::get_if<SavedType*>(&result))
                {
                    saved_ = *saved;
                }
                else if (auto res = std::get_if<ReasoningType>(&result))
                {
                    Base::reasoning = *res;
                }
                else
                {
                    throw std::logic_error("WHATT?");
                }
            }
        }

        template<class T>
        const T& get() const
        {
            return this->Base::template get<T>();
        }

        template<>
        const SavedType& get() const
        {
            return *saved_;
        }
    private:
        SavedType* saved_;
    };

    template<class ReasoningType_, class InputType_>
    struct InputAndReasoning
    {
        explicit InputAndReasoning(const InputType_& input) : input_(input){}

        template<class T>
        const T& get() const;

        template<>
        const InputType_& get() const
        {
            return input_;
        }

        std::optional<ReasoningType_> reasoning{};

    private:
        const InputType_& input_;
    };

    template<class ReasoningType_, class InputType_, class ConcreteDataExtractor>
    struct Extractable<ReasoningType_, InputType_, ConcreteDataExtractor> : public InputAndReasoning<ReasoningType_, InputType_>
    {
        using Base = InputAndReasoning<ReasoningType_, InputType_>;
        using SavedType = typename ConcreteDataExtractor::SavedType;
        using ReasoningType = ReasoningType_;
        Extractable(const InputType_& input,
                    const ConcreteDataExtractor& extractor)
                : Base(input)
        {
            auto result = extractor.extract(*this);

            if (auto saved = std::get_if<SavedType*>(&result))
            {
                saved_ = *saved;
            }
            else if (auto res = std::get_if<ReasoningType>(&result))
            {
                Base::reasoning = *res;
            }
            else
            {
                throw std::logic_error("WHATT?");
            }
        }

        template<class T>
        const T& get() const
        {
            return this->Base::template get<T>();
        }

        template<>
        const SavedType& get() const
        {
            return *saved_;
        }
    private:
        SavedType* saved_;
    };

    template<class T>
    struct ExtractorDataTypes
    {
        using FunctionType = void;
        using Arguments = void;
        using ReturnType = void;
        using SavedType = void;
        using ReasoningType = void;
    };

    template<class ClassType_, class ReturnType_, class ReasoningType_, class... Args_>
    struct ExtractorDataTypes<MaybeValue<ReasoningType_, ReturnType_>(ClassType_::*)(Args_...) const>
    {
        using FunctionType = MaybeValue<ReasoningType_, ReturnType_>(ClassType_::*)(Args_...);
        using Arguments = type_list<Args_...>;
        using ReturnType = ReturnType_;
        using ReasoningType = ReasoningType_;
    };
    
    template<class T>
    struct BuilderDataTypes
    {
        using Arguments = void;
        using ReturnType = void;
    };

    template<class ClassType_, class ReturnType_, class... Args_>
    struct BuilderDataTypes<ReturnType_(ClassType_::*)(Args_...) const>
    {
        using Arguments = type_list<Args_...>;
        using ReturnType = ReturnType_;
    };


    template<class ReturnType_, class Args>
    struct BuilderRunner
    {
        template<class ExtractableType_, class BuilderType_>
        static ReturnType_ run(ExtractableType_ extractable, BuilderType_ builder);
    };

    template<class ReturnType_, class... BuilderArgs>
    struct BuilderRunner<ReturnType_, type_list<BuilderArgs...>>
    {
        template<class ExtractableType_, class BuilderType_>
        static ReturnType_ run(ExtractableType_ extractable, BuilderType_ builder)
        {
            return builder(extractable.template get<std::decay_t<BuilderArgs>>()...);
        }
    };


    template<class ExtractableType_, class BuilderType_>
    auto runBuilder(const ExtractableType_ extractable, BuilderType_ builder)
    {
        return BuilderRunner<
                typename BuilderDataTypes<decltype(&BuilderType_::operator())>::ReturnType,
                typename BuilderDataTypes<decltype(&BuilderType_::operator())>::Arguments>::run(
                        extractable, std::forward<BuilderType_>(builder));
    }


    template <class Extractor_, class... Extractors_>
    struct DataExtractor
    {
        using ReasoningType = typename Extractor_::ReasoningType;
        std::tuple<Extractor_, Extractors_...> concreteExtractors_;
        DataExtractor(Extractor_ ex, Extractors_... exs)
            : concreteExtractors_{ex, exs...}
        {
        }

        template <class InputType_, class Builder_>
        MaybeValue<
                ReasoningType,
                typename BuilderDataTypes<decltype(&Builder_::operator())>::ReturnType>
        extract(const InputType_& input, Builder_ builder)
        {
            Extractable<ReasoningType, InputType_, Extractor_, Extractors_...>
            extractable(input,
                    std::get<Extractor_>(concreteExtractors_), std::get<Extractors_>(concreteExtractors_)...);

            if (extractable.reasoning)
            {
                return *extractable.reasoning;
            }
            else
            {
                return runBuilder(extractable, std::forward<Builder_>(builder));
            }
        }
    };

    template <class... Extractors>
    struct BuildWithReversedExtractors;

    template <>
    struct BuildWithReversedExtractors<>
    {
        template<class... Extractors_>
        static detail::DataExtractor<Extractors_...> build(Extractors_... extractors)
        {
            return {extractors...};
        }
    };


    template <class Extractor, class... ExtractorsPrev_>
    struct BuildWithReversedExtractors<Extractor, ExtractorsPrev_...>
    {
        template <class... ExtractorsNext_>
        static auto build(Extractor extractor, ExtractorsPrev_... extractorsPrev, ExtractorsNext_... extractorsNext)
        {
            return BuildWithReversedExtractors<ExtractorsPrev_...>::build(extractorsPrev..., extractor, extractorsNext...);
        }
    };

    template <class... ConcreteExtractors_>
    auto makeDataExtractorFromConcreteExtractors(ConcreteExtractors_... extractors) -> decltype(auto)
    {
        return BuildWithReversedExtractors<ConcreteExtractors_...>::build(extractors...);
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
        typename detail::ExtractorDataTypes<decltype(&ExtractorType_::operator())>::ReturnType ,
        typename detail::ExtractorDataTypes<decltype(&ExtractorType_::operator())>::ReasoningType ,
        ExtractorType_,
        typename detail::ExtractorDataTypes<decltype(&ExtractorType_::operator())>::Arguments>
makeExtractor(ExtractorType_ extractor)
{
    return {extractor};
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
auto makeExtractableRev(InputType_ input, Extractors_... extractors) -> decltype(auto)
{
    return makeExtractableReversed(input, makeExtractor(extractors)...);
}


template <class InputType_, class... Extractors_>
auto makeExtractable(InputType_ input, Extractors_... extractors) -> decltype(auto)
{
    return makeExtractableDetailed(input, makeExtractor(extractors)...);
}

template <class... Extractors_>
auto makeDataExtractor(Extractors_... extractors) -> decltype(auto)
{
    return detail::makeDataExtractorFromConcreteExtractors(makeExtractor(extractors)...);
}


#endif //DATAEXTRACTOR_DATAEXTRACTOR_HPP
