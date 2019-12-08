#include <memory>
#include <functional>
#include <variant>
#include <optional>
#include <type_traits>

#include <DataExtractor.hpp>

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
using MaybeResult = MaybeValue<ReasoningStatus, T>;

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


std::string builderFunction(std::string str, double dbl)
{
    std::cout << "I HAVE BUILD IT " << str << " dbl: " << dbl;
    return str + "eeeee?";
}

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

    static_assert(std::is_same_v<detail::ExtractorDataTypes<decltype(&doubleExtractor::operator())>::ReasoningType, ReasoningStatus>);

    auto extractableRes = makeExtractableRev(
                    in2,
                    stringExtractor,
                    intExtractor,
                    doubleExtractor{}
                    );

//    auto extractableRes = makeExtractable(
//            in2,
//            doubleExtractor{},
//            intExtractor,
//            stringExtractor
//    );

    std::cout<<"extractable reasoning is " << static_cast<bool>(extractableRes.reasoning) << std::endl;

    std::cout<<"Extractable double : " <<  extractableRes.get<double>() << std::endl;
    std::cout<<"Extractable int : " <<  extractableRes.get<int>() << std::endl;
    std::cout<<"Extractable string : " <<  extractableRes.get<std::string>() << std::endl;


    auto dataExtractor = makeDataExtractor(stringExtractor, intExtractor, doubleExtractor{});


    auto builder = [](
            const InputStruct& inputStruct,
            const double& dbl,
            const std::string& str,
            int intiger) -> std::string
    {
        std::cout<<"Helloooo " << str<< " dbl: "<< dbl << " integer : " << intiger;
        return std::string("Great success");
    };
    auto result = dataExtractor.extract(in2, builder);
    auto result2 = dataExtractor.extract(in2, &builderFunction);


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

template <class T>
using StringOrResult = MaybeValue<std::string, T>;

TEST(ExampleUsages, case1)
{
    double val = 7.1;
    auto extractor = makeDataExtractor(
            [&](const int& firstValue) -> StringOrResult<double*> { return &val;},
            [](const double& secValue) -> StringOrResult<float*> {return "FAIL";});

    auto result = extractor.extract(32, [](){ std::cout<<"Successfull extraction" << std::endl; return true;});

    std::visit([](auto&& arg){std::cout<<"RESULT OF EXTRACTION: " << arg << std::endl;}, result); // FAIL will be printed
}

TEST(ExampleUsages, case2)
{
    double val = 7.1;
    float valF = 3.1;

    auto extractor = makeDataExtractor(
            [&](const int& firstValue) -> StringOrResult<double*> { return &val;},
            [&](const double& secValue) -> StringOrResult<float*> {return &valF;});
    auto result = extractor.extract(32, [](){ std::cout<<"Successfull extraction" << std::endl; return true;});

    std::visit([](auto&& arg){std::cout<<"RESULT OF EXTRACTION: " << arg << std::endl;}, result); // True will be printed
}
