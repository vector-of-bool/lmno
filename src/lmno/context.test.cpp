#include "./context.hpp"

#include <string_view>

using namespace lmno;

namespace {

constexpr scope<> s1 = {};

static_assert(not s1.has_name_v<"dog">);

constexpr auto s2 = s1.bind(make_named<"dog">(12));

static_assert(s2.has_name_v<"dog">);

static_assert(s2.get<"dog">() == 12);

constexpr auto s3 = s2.bind(make_named<"cat">(std::string_view("I am a string")));

static_assert(s3.get<"cat">() == "I am a string");

constexpr auto s4 = s3.bind(make_named<"dog">('a'));
static_assert(s4.get<"dog">() == 'a');
static_assert(s4.get<"cat">() == "I am a string");

struct nonempty {
    std::plus<> p;
};

static_assert(stateless<scope<named_value<"f", nonempty>>>);

}  // namespace
