#pragma once

namespace Fancysoft::Phoenix::Util::Lambda {

template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace Fancysoft::Phoenix::Util::Lambda
