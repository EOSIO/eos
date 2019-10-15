#pragma once

#include <multibase/detail/codec_impl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace multibase {

template <encoding T>
struct traits {
  static const std::array<char, 0> charset;
  static const char name[];
};

/** Template implementation of base encoding which computes a lookup table at
 * compile time and avoids the virtual function lookup penalty */
template <encoding T, typename Traits = traits<T>>
class basic_codec : public codec::impl {
 public:
  using this_type = basic_codec<T, Traits>;
  using codec::impl::decode;
  using codec::impl::encode;
  using codec::impl::is_valid;
  encoding get_encoding() override;
  bool is_valid(std::string_view input, impl_tag) override;
  std::size_t get_encoded_size(std::string_view input) override;
  std::size_t encode(std::string_view input, std::string& output,
                     impl_tag) override;
  std::size_t get_decoded_size(std::string_view input) override;
  std::size_t decode(std::string_view input, std::string& output,
                     impl_tag) override;

 private:
  constexpr static auto first = Traits::charset.cbegin();
  constexpr static auto last = Traits::charset.cend();
  using CharsetT = decltype(Traits::charset);
  using value_type = typename CharsetT::value_type;
  using iterator = typename CharsetT::const_iterator;

  /** Find a value at compile time */
  constexpr static iterator find(iterator b, iterator e,
                                 value_type const& v) noexcept {
    return (b != e && *b != v) ? find(++b, e, v) : b;
  }

  /** Determine the character encoding for a given value
  @return character encoding, or 0 if none such encoding exists */
  constexpr static unsigned char getval(unsigned char p) noexcept {
    return find(first, last, p) == last
               ? static_cast<unsigned char>(255)
               : static_cast<unsigned char>(
                     std::distance(first, find(first, last, p)));
  }

  /** Compute base-2 logarithm */
  constexpr static float log2(float n) noexcept {
    return (n == 1) ? 0 : ((n < 2) ? 1 : 1 + log2(n / 2));
  }

  /** encoding as determined by size of character set */
  constexpr static auto radix = sizeof(Traits::charset) / sizeof(value_type);
  /** Ratio of encoded characters per byte */
  constexpr static auto ratio = log2(256) / log2(radix);
  /** Map from value to corresponding character in base encoding */
  static const std::array<unsigned char, 256> valset;
};

template <encoding T, typename Traits>
constexpr const std::array<unsigned char, 256> basic_codec<T, Traits>::valset =
    {getval(0),   getval(1),   getval(2),   getval(3),   getval(4),
     getval(5),   getval(6),   getval(7),   getval(8),   getval(9),
     getval(10),  getval(11),  getval(12),  getval(13),  getval(14),
     getval(15),  getval(16),  getval(17),  getval(18),  getval(19),
     getval(20),  getval(21),  getval(22),  getval(23),  getval(24),
     getval(25),  getval(26),  getval(27),  getval(28),  getval(29),
     getval(30),  getval(31),  getval(32),  getval(33),  getval(34),
     getval(35),  getval(36),  getval(37),  getval(38),  getval(39),
     getval(40),  getval(41),  getval(42),  getval(43),  getval(44),
     getval(45),  getval(46),  getval(47),  getval(48),  getval(49),
     getval(50),  getval(51),  getval(52),  getval(53),  getval(54),
     getval(55),  getval(56),  getval(57),  getval(58),  getval(59),
     getval(60),  getval(61),  getval(62),  getval(63),  getval(64),
     getval(65),  getval(66),  getval(67),  getval(68),  getval(69),
     getval(70),  getval(71),  getval(72),  getval(73),  getval(74),
     getval(75),  getval(76),  getval(77),  getval(78),  getval(79),
     getval(80),  getval(81),  getval(82),  getval(83),  getval(84),
     getval(85),  getval(86),  getval(87),  getval(88),  getval(89),
     getval(90),  getval(91),  getval(92),  getval(93),  getval(94),
     getval(95),  getval(96),  getval(97),  getval(98),  getval(99),
     getval(100), getval(101), getval(102), getval(103), getval(104),
     getval(105), getval(106), getval(107), getval(108), getval(109),
     getval(110), getval(111), getval(112), getval(113), getval(114),
     getval(115), getval(116), getval(117), getval(118), getval(119),
     getval(120), getval(121), getval(122), getval(123), getval(124),
     getval(125), getval(126), getval(127), getval(128), getval(129),
     getval(130), getval(131), getval(132), getval(133), getval(134),
     getval(135), getval(136), getval(137), getval(138), getval(139),
     getval(140), getval(141), getval(142), getval(143), getval(144),
     getval(145), getval(146), getval(147), getval(148), getval(149),
     getval(150), getval(151), getval(152), getval(153), getval(154),
     getval(155), getval(156), getval(157), getval(158), getval(159),
     getval(160), getval(161), getval(162), getval(163), getval(164),
     getval(165), getval(166), getval(167), getval(168), getval(169),
     getval(170), getval(171), getval(172), getval(173), getval(174),
     getval(175), getval(176), getval(177), getval(178), getval(179),
     getval(180), getval(181), getval(182), getval(183), getval(184),
     getval(185), getval(186), getval(187), getval(188), getval(189),
     getval(190), getval(191), getval(192), getval(193), getval(194),
     getval(195), getval(196), getval(197), getval(198), getval(199),
     getval(200), getval(201), getval(202), getval(203), getval(204),
     getval(205), getval(206), getval(207), getval(208), getval(209),
     getval(210), getval(211), getval(212), getval(213), getval(214),
     getval(215), getval(216), getval(217), getval(218), getval(219),
     getval(220), getval(221), getval(222), getval(223), getval(224),
     getval(225), getval(226), getval(227), getval(228), getval(229),
     getval(230), getval(231), getval(232), getval(233), getval(234),
     getval(235), getval(236), getval(237), getval(238), getval(239),
     getval(240), getval(241), getval(242), getval(243), getval(244),
     getval(245), getval(246), getval(247), getval(248), getval(249),
     getval(250), getval(251), getval(252), getval(253), getval(254),
     getval(255)};

template <encoding T, typename Traits>
encoding basic_codec<T, Traits>::get_encoding() {
  return T;
}

template <encoding T, typename Traits>
std::size_t basic_codec<T, Traits>::get_encoded_size(
    std::string_view input) {
  return static_cast<std::size_t>(std::ceil(input.size() * ratio) + 1);
}

template <encoding T, typename Traits>
std::size_t basic_codec<T, Traits>::encode(std::string_view input,
                                           std::string& output, impl_tag) {
  auto not_zero = [](auto c) { return c != 0; };
  auto ii = std::find_if(std::begin(input), std::end(input), not_zero);
  auto zeroes = std::distance(std::begin(input), ii);
  int length = 0;
  for (auto end = std::end(input); ii != end; ++ii) {
    int carry = static_cast<unsigned char>(*ii);
    int i = 0;
    for (auto oi = output.rbegin();
         (oi != output.rend()) && (carry != 0 || i < length); ++oi, ++i) {
      carry += 256 * (*oi);
      auto byte = (unsigned char*)(&(*oi));
      *byte = carry % radix;
      carry /= radix;
    }
    length = i;
  }
  auto it = std::find_if(output.begin() + (output.size() - length),
                         output.end(), [](auto c) { return c != 0; });
  auto size = std::distance(it, output.end()) + zeroes;
  std::fill_n(output.begin(), zeroes, Traits::charset[0]);
  std::transform(it, output.end(), &output[zeroes],
                 [](auto c) { return Traits::charset[c]; });
  return static_cast<size_t>(size);
}

template <encoding T, typename Traits>
std::size_t basic_codec<T, Traits>::get_decoded_size(
    std::string_view input) {
  auto not_zero = [](auto c) { return c != Traits::charset[0]; };
  auto begin = input.begin(), end = input.end();
  auto zeroes = std::distance(begin, std::find_if(begin, end, not_zero));
  return std::size_t(std::ceil((input.size() - zeroes) / ratio) + zeroes);
}

template <encoding T, typename Traits>
std::size_t basic_codec<T, Traits>::decode(std::string_view input,
                                           std::string& output, impl_tag) {
  auto not_zero = [](auto c) { return c != Traits::charset[0]; };
  auto ii = std::find_if(input.begin(), input.end(), not_zero);
  auto zeroes = std::distance(input.begin(), ii);
  auto length = 0;
  for (auto end = input.end(); ii != end && *ii; ++ii) {
    int carry = valset[(unsigned char)(*ii)];
    if (carry == 255) throw std::range_error("Invalid input character");
    int i = 0;
    for (auto oi = output.rbegin();
         (carry != 0 || i < length) && (oi != output.rend()); ++oi, ++i) {
      auto byte = (unsigned char*)(&(*oi));
      carry += radix * (*byte);
      *byte = static_cast<unsigned char>(carry % 256);
      carry /= 256;
    }
    length = i;
  }
  auto it = std::find_if(output.begin() + (output.size() - length),
                         output.end(), not_zero);
  auto output_size = zeroes + std::distance(it, output.end());
  auto oit = const_cast<char*>(&output[zeroes]);
  std::copy(it, output.end(), oit);
  std::fill_n(const_cast<char*>(&output[0]), zeroes, 0);
  return static_cast<size_t>(output_size);
}

template <encoding T, typename Traits>
bool basic_codec<T, Traits>::is_valid(std::string_view input, impl_tag) {
  return input.end() == std::find_if(input.begin(), input.end(),
                                     [](auto c) { return valset[c] == 255; });
}

template <>
struct traits<encoding::base_16> {
  constexpr static const std::array<char, 16> charset = {
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  constexpr static const char name[] = "base_16";
};
using base_16 = basic_codec<encoding::base_16>;

template <>
struct traits<encoding::base_58_btc> {
  constexpr static const std::array<char, 58> charset = {
      '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
      'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
      'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
  constexpr static const char name[] = "base_58_btc";
};
using base_58_btc = basic_codec<encoding::base_58_btc>;

}  // namespace multibase
