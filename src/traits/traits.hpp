#include <optional>
#include <variant>
namespace gbg {
template <class... Ts>
struct overloads : Ts... {
    using Ts::operator()...;
};

template <typename T, class ... Ts>
std::optional<T> get_op(std::variant<Ts...>& vt) {
    if(std::holds_alternative<T>(vt)) {
        return std::get<T>(vt);
    }
    return std::nullopt;
}

}  // namespace gbg
