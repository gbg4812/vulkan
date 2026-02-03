namespace gbg {
template <class... Ts>
struct overloads : Ts... {
    using Ts::operator()...;
};

}  // namespace gbg
