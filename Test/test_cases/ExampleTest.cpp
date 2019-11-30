#include <memory>
#include <functional>
#include <variant>
#include <optional>
#include <type_traits>
#include "gmock/gmock.h"

struct StructA
{
    int value = 7;
};

struct StructB
{
    int value = 9;
};


int fun1(const StructA& a)
{
    std::cout<<"Calling fun1"<< std::endl;
    return 7;
}


void fun2(const StructB& b)
{
    std::cout<<"Calling fun2"<< std::endl;
}


void fun3(const StructA& a, const StructB& b)
{
    std::cout<<"Calling fun3"<< std::endl;
}


struct FunctionaStruct{
    void operator()(const StructB& b) const
    {
        std::cout<<"Calling fun5" << std::endl;
    }
};

template <class ReturnType, class ContainerType, class... Args>
ReturnType callFunction(const ContainerType& container, ReturnType(*toCall)(Args...))
{
    return toCall(std::get<typename std::remove_const_t<std::remove_reference_t<Args>>>(container)...);
}

template <class ReturnType, class ContainerType, class... Args>
ReturnType callFunction(const ContainerType& container, std::function<ReturnType(Args...)> toCall)
{
    return toCall(std::get<typename std::remove_const_t<std::remove_reference_t<Args>>>(container)...);
}


using namespace testing;
TEST(DataExtractor, extractionOfParameters)
{
    std::pair<StructA,StructB> somePair;

//    auto fun4 = [](const StructA& a)
//    {
//        std::cout<<"Calling fun4"<< std::endl;
//    };


    std::cout<<std::endl;
    callFunction(somePair, fun1);
    callFunction(somePair, fun2);
    callFunction(somePair, fun3);


    std::function<void(const StructA&)> fun4 = [](const StructA& a)
    {
        std::cout<<"Calling fun4"<< std::endl;
    };

    callFunction(somePair, fun4);

    std::function<void(const StructB&)> fun5 = FunctionaStruct{};
    callFunction(somePair, fun5);
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct InputStruct
{
    int value;
};

struct ResultStructure
{
    const int& a;
    const double& b;
    const std::string& c;
};

enum class ReasoningStatus
{
    Fail1,
    Fail2
};

template <class T>
using MaybeResult = std::variant<ReasoningStatus, T>;

MaybeResult<int*> intExtractor(const InputStruct& in, const std::string& strValue)
{
    std::cout<<"inT extractor Got string value" << strValue << std::endl;
    if(in.value > 10)
    {
        static int res = 8;
        std::cout<<"int extractor returning value" << std::endl;
        return &res;
    } else
    {
        std::cout<<"int extractor returning fail" << std::endl;
        return ReasoningStatus::Fail1;
    }
}


struct doubleExtractor
{
    explicit doubleExtractor(double partPoint = 15.0)
        : partPoint_(partPoint)
    {

    }

    MaybeResult<double*> operator()(const InputStruct& in, const int& intValue, const std::string& strValue) const
    {
        std::cout<<"Double extractor Got int value " << intValue << std::endl;
        std::cout<<"Double extractor Got string value " << strValue << std::endl;
        if(in.value < 15)
        {
            static double res = 8;

            std::cout<<"double extractor returning value" << std::endl;

            return &res;
        } else
        {
            std::cout<<"double extractor returning fail" << std::endl;
            return ReasoningStatus::Fail2;
        }
    }
private:
    double partPoint_;
};

template <typename... Args>
struct type_list {};


template<class ReturnType_, class ReasoningType_, class InputType_, class... Args_>
struct ConcreteFunctionExtractor
{
    using FunctionType = std::variant<ReasoningType_, ReturnType_>(*)(const InputType_&, Args_...);
    using InputType = InputType_;
    using Arguments = type_list<Args_...>;
    using ReturnType = ReturnType_;
    using SavedType = typename std::remove_pointer_t<ReturnType_>;
    using ReasoningType = ReasoningType_;

    FunctionType extractor_ ;

    template <class InputType, class GetableContainer>
    std::variant<ReasoningType_, ReturnType_> extract(const InputType& input, const GetableContainer& container) const
    {
        return extractor_(input, container.template get<typename std::decay_t<Args_>>()...);
    }
};


template<class ReturnType_, class ReasoningType_, class Extractor, class InputType_, class... Args_>
struct ConcreteExtractor
{
    using InputType = InputType_;
    using Arguments = type_list<Args_...>;
    using ReturnType = ReturnType_;
    using SavedType = typename std::remove_pointer_t<ReturnType_>;
    using ReasoningType = ReasoningType_;

    Extractor extractor;

    template <class InputType, class GetableContainer>
    auto extract(const InputType& input, const GetableContainer& container) const
    {
        return extractor(input, container.template get<typename std::decay_t<Args_>>()...);
    }
};

template<class ReturnType_, class ReasoningType_, class Extractor, class InputType_, class... Args_>
struct ConcreteExtractor<ReturnType_, ReasoningType_, Extractor, InputType_, type_list<Args_...>>
{
    using InputType = InputType_;
    using Arguments = type_list<Args_...>;
    using ReturnType = ReturnType_;
    using SavedType = typename std::remove_pointer_t<ReturnType_>;
    using ReasoningType = ReasoningType_;

    Extractor extractor;


    template <class InputType, class GetableContainer>
    auto extract(const InputType& input, const GetableContainer& container) const
    {
        return extractor(input, container.template get<typename std::decay_t<Args_>>()...);
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
            auto result = extractor.extract(input, static_cast<const Base&>(*this));
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

template<class ReasoningType_, class InputType, class ConcreteDataExtractor>
struct Extractable<ReasoningType_, InputType, ConcreteDataExtractor>
{
    using SavedType = typename ConcreteDataExtractor::SavedType;
    using ReasoningType = ReasoningType_;
    Extractable(const InputType& input,
                const ConcreteDataExtractor& extractor)
    {
        auto result = extractor.extractor(input);

        if (auto saved = std::get_if<SavedType*>(&result))
        {
            saved_ = *saved;
        }
        else if (auto res = std::get_if<ReasoningType>(&result))
        {
            reasoning = *res;
        }
        else
        {
            throw std::logic_error("WHATT?");
        }
    }

    template<class T>
    const T& get() const;

    template<>
    const SavedType& get() const
    {
        return *saved_;
    }


    std::optional<ReasoningType> reasoning{};

private:
    SavedType* saved_;
};



template <class ReturnType_, class ReasoningType_, class InputType_, class... Args_ >
struct ExtractorTypeWrapper
{

};


template <class ReturnType_, class ReasoningType_, class InputType_, class... Args_>
ConcreteFunctionExtractor<
        ReturnType_,
        ReasoningType_,
        typename std::decay_t<InputType_>,
        Args_...>
        makeExtractor(std::variant<ReasoningType_, ReturnType_>(inputFunction)(InputType_, Args_...))
{
    return {inputFunction};
}

template<class T>
struct FunctionDataTypes
{
    using FunctionType = void;
    using InputType = void;
    using Arguments = void;
    using ReturnType = void;
    using SavedType = void;
    using ReasoningType = void;
};

template<class ClassType_, class ReturnType_, class ReasoningType_, class InputType_, class... Args_>
struct FunctionDataTypes<std::variant<ReasoningType_, ReturnType_>(ClassType_::*)(const InputType_&, Args_...) const>
{
    using FunctionType = std::variant<ReasoningType_, ReturnType_>(ClassType_::*)(const InputType_&, Args_...);
    using InputType = InputType_;
    using Arguments = type_list<Args_...>;
    using ReturnType = ReturnType_;
    using SavedType = typename std::remove_pointer_t<ReturnType_>;
    using ReasoningType = ReasoningType_;
};


template <class ExtractorType_>
ConcreteExtractor<
        typename FunctionDataTypes<decltype(&ExtractorType_::operator())>::SavedType ,
        typename FunctionDataTypes<decltype(&ExtractorType_::operator())>::ReasoningType ,
        ExtractorType_,
        typename FunctionDataTypes<decltype(&ExtractorType_::operator())>::InputType,
        typename FunctionDataTypes<decltype(&ExtractorType_::operator())>::Arguments>
makeExtractor(ExtractorType_ extractor)
{
    return {extractor};
}


template <class InputType_, class Extractor_, class... Extractors_>
Extractable<typename Extractor_::ReasoningType, InputType_, Extractor_, Extractors_...>
makeExtractableDetailed(InputType_ input, Extractor_ anyExtractor, Extractors_... extractors)
{
    return {input, anyExtractor, extractors...};
}

template <class InputType_, class... Extractors_>
auto makeExtractable(InputType_ input, Extractors_... extractors) -> decltype(auto)
{
    return makeExtractableDetailed(input, makeExtractor(extractors)...);
}







class PartExtractor
{

};


//template<class... Args>
//struct DataExtractor
//{
//    std::tuple<Args...> partExtractors_;
//
//
//    template<class ReturnType, class InputType, class Builder>
//    MaybeResult<ReturnType> build(const InputType& input, const Builder& builder)
//    {
//        Extractable extractable(input, partExtractors_...);
//        if (!extractable.reasoning)
//        {
//            return builder(extractors);
//        }
//        else
//        {
//            return extractable.reasoning;
//        }
//    }
//};
//

TEST(DataExtraction, firstTries)
{

    std::string someString = "someString";

    auto stringExtractor = [&someString](const InputStruct& in)->MaybeResult<std::string*>
    {
        if(in.value < 12)
        {
            std::cout<<"string extractor returning value" << std::endl;

            return &someString;
        } else
        {
            std::cout<<"string extractor returning fail" << std::endl;
            return ReasoningStatus::Fail1;
        }
    };

    InputStruct in1{3};
    InputStruct in2{11};

    auto someRes = intExtractor(in1, "A");
    auto someRes2 = intExtractor(in2, "b");

//    doubleExtractor{}(in1);
//    doubleExtractor{}(in2);

    stringExtractor(in1);
    stringExtractor(in2);

    std::visit(overloaded{
            [](int *) { std::cout << "int pointer obtained" << std::endl; },
            [](const ReasoningStatus &) { std::cout << "reasoning obtained"<< std::endl; },
    }, someRes);

    std::visit(overloaded{
            [](int *) { std::cout << "int pointer obtained"<< std::endl; },
            [](const ReasoningStatus &) { std::cout << "reasoning obtained"<< std::endl; },
    }, someRes2);


//    Extractable<
//            ReasoningStatus,
//            InputStruct,
//            ConcreteExtractor<double, doubleExtractor, int, std::string>,
//            ConcreteExtractor<int, decltype(intExtractor), std::string>,
//            ConcreteExtractor<std::string, decltype(stringExtractor)>>
//        extractableRes{in2,
//                       ConcreteExtractor<double, doubleExtractor, int, std::string>{doubleExtractor{}},
//                       ConcreteExtractor<int, decltype(intExtractor), std::string>{intExtractor},
//                       ConcreteExtractor<std::string, decltype(stringExtractor)>{stringExtractor}};

    static_assert(std::is_same_v<FunctionDataTypes<decltype(&doubleExtractor::operator())>::ReasoningType, ReasoningStatus>);

    auto extractableRes = makeExtractable(
                    in2,
                    doubleExtractor{},
                    intExtractor,
                    stringExtractor
                    );

    std::cout<<"extractable reasoning is " << static_cast<bool>(extractableRes.reasoning) << std::endl;

    std::cout<<"Extractable double : " <<  extractableRes.get<double>() << std::endl;
    std::cout<<"Extractable int : " <<  extractableRes.get<int>() << std::endl;
    std::cout<<"Extractable string : " <<  extractableRes.get<std::string>() << std::endl;

//    DataExtractor{
//        intExtractor,
//        doubleExtractor,
//        stringExtractor
//    } extractor;

//    auto builder = [](const auto& in){
//        return ResultStructure{
//            in.get<int>(),
//            in.get<double>(),
//            in.get<std::string>()
//        };
//    };
//    extractor.build(in1, builder);
}
