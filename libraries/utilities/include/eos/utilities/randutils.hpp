/*
 * Random-Number Utilities (randutil)
 *     Addresses common issues with C++11 random number generation.
 *     Makes good seeding easier, and makes using RNGs easy while retaining
 *     all the power.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Melissa E. O'Neill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef RANDUTILS_HPP
#define RANDUTILS_HPP 1

/*
 * This header includes three class templates that can help make C++11
 * random number generation easier to use.
 *
 * randutils::seed_seq_fe
 *
 *   Fixed-Entropy Seed sequence
 *
 *   Provides a replacement for std::seed_seq that avoids problems with bias,
 *   performs better in empirical statistical tests, and executes faster in
 *   normal-sized use cases.
 *
 *   In normal use, it's accessed via one of the following type aliases
 *
 *       randutils::seed_seq_fe128
 *       randutils::seed_seq_fe256
 *
 *   It's discussed in detail at
 *       http://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html
 *   and the motivation for its creation (what's wrong with std::seed_seq) here
 *       http://www.pcg-random.org/posts/cpp-seeding-surprises.html
 *
 *
 * randutils::auto_seeded
 *
 *   Extends a seed sequence class with a nondeterministic default constructor.
 *   Uses a variety of local sources of entropy to portably initialize any
 *   seed sequence to a good default state.
 *
 *   In normal use, it's accessed via one of the following type aliases, which
 *   use seed_seq_fe128 and seed_seq_fe256 above.
 *
 *       randutils::auto_seed_128
 *       randutils::auto_seed_256
 *
 *   It's discussed in detail at
 *       http://www.pcg-random.org/posts/simple-portable-cpp-seed-entropy.html
 *   and its motivation (why you can't just use std::random_device) here
 *       http://www.pcg-random.org/posts/cpps-random_device.html
 *
 *
 * randutils::random_generator
 *
 *   An Easy-to-Use Random API
 *
 *   Provides all the power of C++11's random number facility in an easy-to
 *   use wrapper.
 *
 *   In normal use, it's accessed via one of the following type aliases, which
 *   also use auto_seed_256 by default
 *
 *       randutils::default_rng
 *       randutils::mt19937_rng
 *
 *   It's discussed in detail at
 *       http://www.pcg-random.org/posts/ease-of-use-without-loss-of-power.html
 */

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <array>
#include <functional>  // for std::hash
#include <initializer_list>
#include <utility>
#include <type_traits>
#include <iterator>
#include <chrono>
#include <thread>
#include <algorithm>

// Ugly platform-specific code for auto_seeded

#if !defined(RANDUTILS_CPU_ENTROPY) && defined(__has_builtin)
    #if __has_builtin(__builtin_readcyclecounter)
        #define RANDUTILS_CPU_ENTROPY __builtin_readcyclecounter()
    #endif
#endif
#if !defined(RANDUTILS_CPU_ENTROPY)
    #if __i386__
        #if __GNUC__
            #define RANDUTILS_CPU_ENTROPY __builtin_ia32_rdtsc()
        #else
            #include <immintrin.h>
            #define RANDUTILS_CPU_ENTROPY __rdtsc()
        #endif
    #else
        #define RANDUTILS_CPU_ENTROPY 0
    #endif
#endif

#if defined(RANDUTILS_GETPID)
    // Already defined externally
#elif defined(_WIN64) || defined(_WIN32)
    #include <process.h>
    #define RANDUTILS_GETPID _getpid()
#elif defined(__unix__) || defined(__unix) \
      || (defined(__APPLE__) && defined(__MACH__))
    #include <unistd.h>
    #define RANDUTILS_GETPID getpid()
#else
    #define RANDUTILS_GETPID 0
#endif

#if __cpp_constexpr >= 201304L
    #define RANDUTILS_GENERALIZED_CONSTEXPR constexpr
#else
    #define RANDUTILS_GENERALIZED_CONSTEXPR
#endif



namespace randutils {

//////////////////////////////////////////////////////////////////////////////
//
// seed_seq_fe
//
//////////////////////////////////////////////////////////////////////////////

/*
 * seed_seq_fe implements a fixed-entropy seed sequence; it conforms to all
 * the requirements of a Seed Sequence concept.
 *
 * seed_seq_fe<N> implements a seed sequence which seeds based on a store of
 * N * 32 bits of entropy.  Typically, it would be initialized with N or more
 * integers.
 *
 * seed_seq_fe128 and seed_seq_fe256 are provided as convenience typedefs for
 * 128- and 256-bit entropy stores respectively.  These variants outperform
 * std::seed_seq, while being better mixing the bits it is provided as entropy.
 * In almost all common use cases, they serve as better drop-in replacements
 * for seed_seq.
 *
 * Technical details
 *
 * Assuming it constructed with M seed integers as input, it exhibits the
 * following properties
 *
 * * Diffusion/Avalanche:  A single-bit change in any of the M inputs has a
 *   50% chance of flipping every bit in the bitstream produced by generate.
 *   Initializing the N-word entropy store with M words requires O(N * M)
 *   time precisely because of the avalanche requirements.  Once constructed,
 *   calls to generate are linear in the number of words generated.
 *
 * * Bias freedom/Bijection: If M == N, the state of the entropy store is a
 *   bijection from the M inputs (i.e., no states occur twice, none are
 *   omitted). If M > N the number of times each state can occur is the same
 *   (each state occurs 2**(32*(M-N)) times, where ** is the power function).
 *   If M < N, some states cannot occur (bias) but no state occurs more
 *   than once (it's impossible to avoid bias if M < N; ideally N should not
 *   be chosen so that it is more than M).
 *
 *   Likewise, the generate function has similar properties (with the entropy
 *   store as the input data).  If more outputs are requested than there is
 *   entropy, some outputs cannot occur.  For example, the Mersenne Twister
 *   will request 624 outputs, to initialize it's 19937-bit state, which is
 *   much larger than a 128-bit or 256-bit entropy pool.  But in practice,
 *   limiting the Mersenne Twister to 2**128 possible initializations gives
 *   us enough initializations to give a unique initialization to trillions
 *   of computers for billions of years.  If you really have 624 words of
 *   *real* high-quality entropy you want to use, you probably don't need
 *   an entropy mixer like this class at all.  But if you *really* want to,
 *   nothing is stopping you from creating a randutils::seed_seq_fe<624>.
 *
 * * As a consequence of the above properties, if all parts of the provided
 *   seed data are kept constant except one, and the remaining part is varied
 *   through K different states, K different output sequences will be produced.
 *
 * * Also, because the amount of entropy stored is fixed, this class never
 *   performs dynamic allocation and is free of the possibility of generating
 *   an exception.
 *
 * Ideas used to implement this code include hashing, a simple PCG generator
 * based on an MCG base with an XorShift output function and permutation
 * functions on tuples.
 *
 * More detail at
 *     http://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html
 */

template <size_t count = 4, typename IntRep = uint32_t,
          size_t mix_rounds = 1 + (count <= 2)>
struct seed_seq_fe {
public:
    // types
    typedef IntRep result_type;

private:
    static constexpr uint32_t INIT_A = 0x43b0d7e5;
    static constexpr uint32_t MULT_A = 0x931e8875;

    static constexpr uint32_t INIT_B = 0x8b51f9dd;
    static constexpr uint32_t MULT_B = 0x58f38ded;

    static constexpr uint32_t MIX_MULT_L = 0xca01f9dd;
    static constexpr uint32_t MIX_MULT_R = 0x4973f715;
    static constexpr uint32_t XSHIFT = sizeof(IntRep)*8/2;

    RANDUTILS_GENERALIZED_CONSTEXPR
    static IntRep fast_exp(IntRep x, IntRep power)
    {
        IntRep result = IntRep(1);
        IntRep multiplier = x;
        while (power != IntRep(0)) {
            IntRep thismult = power & IntRep(1) ? multiplier : IntRep(1);
            result *= thismult;
            power >>= 1;
            multiplier *= multiplier;
        }
        return result;
    }

    std::array<IntRep, count> mixer_;

    template <typename InputIter>
    void mix_entropy(InputIter begin, InputIter end);

public:
    seed_seq_fe(const seed_seq_fe&)     = delete;
    void operator=(const seed_seq_fe&)  = delete;

    template <typename T>
    seed_seq_fe(std::initializer_list<T> init)
    {
        seed(init.begin(), init.end());
    }

    template <typename InputIter>
    seed_seq_fe(InputIter begin, InputIter end)
    {
        seed(begin, end);
    }

    // generating functions
    template <typename RandomAccessIterator>
    void generate(RandomAccessIterator first, RandomAccessIterator last) const;

    static constexpr size_t size()
    {
        return count;
    }

    template <typename OutputIterator>
    void param(OutputIterator dest) const;

    template <typename InputIter>
    void seed(InputIter begin, InputIter end)
    {
        mix_entropy(begin, end);
        // For very small sizes, we do some additional mixing.  For normal
        // sizes, this loop never performs any iterations.
        for (size_t i = 1; i < mix_rounds; ++i)
            stir();
    }

    seed_seq_fe& stir()
    {
        mix_entropy(mixer_.begin(), mixer_.end());
        return *this;
    }

};

template <size_t count, typename IntRep, size_t r>
template <typename InputIter>
void seed_seq_fe<count, IntRep, r>::mix_entropy(InputIter begin, InputIter end)
{
    auto hash_const = INIT_A;
    auto hash = [&](IntRep value) {
        value ^= hash_const;
        hash_const *= MULT_A;
        value *= hash_const;
        value ^= value >> XSHIFT;
        return value;
    };
    auto mix = [](IntRep x, IntRep y) {
        IntRep result = MIX_MULT_L*x - MIX_MULT_R*y;
        result ^= result >> XSHIFT;
        return result;
    };

    InputIter current = begin;
    for (auto& elem : mixer_) {
        if (current != end)
            elem = hash(*current++);
        else
            elem = hash(0U);
    }
    for (auto& src : mixer_)
        for (auto& dest : mixer_)
            if (&src != &dest)
                dest = mix(dest,hash(src));
    for (; current != end; ++current)
        for (auto& dest : mixer_)
            dest = mix(dest,hash(*current));
}

template <size_t count, typename IntRep, size_t mix_rounds>
template <typename OutputIterator>
void seed_seq_fe<count,IntRep,mix_rounds>::param(OutputIterator dest) const
{
    const IntRep INV_A = fast_exp(MULT_A, IntRep(-1));
    const IntRep MIX_INV_L = fast_exp(MIX_MULT_L, IntRep(-1));

    auto mixer_copy = mixer_;
    for (size_t round = 0; round < mix_rounds; ++round) {
        // Advance to the final value.  We'll backtrack from that.
        auto hash_const = INIT_A*fast_exp(MULT_A, IntRep(count * count));

        for (auto src = mixer_copy.rbegin(); src != mixer_copy.rend(); ++src)
            for (auto dest = mixer_copy.rbegin(); dest != mixer_copy.rend();
                 ++dest)
                if (src != dest) {
                    IntRep revhashed = *src;
                    auto mult_const = hash_const;
                    hash_const *= INV_A;
                    revhashed ^= hash_const;
                    revhashed *= mult_const;
                    revhashed ^= revhashed >> XSHIFT;
                    IntRep unmixed = *dest;
                    unmixed ^= unmixed >> XSHIFT;
                    unmixed += MIX_MULT_R*revhashed;
                    unmixed *= MIX_INV_L;
                    *dest = unmixed;
                }
        for (auto i = mixer_copy.rbegin(); i != mixer_copy.rend(); ++i) {
            IntRep unhashed = *i;
            unhashed ^= unhashed >> XSHIFT;
            unhashed *= fast_exp(hash_const, IntRep(-1));
            hash_const *= INV_A;
            unhashed ^= hash_const;
            *i = unhashed;
        }
    }
    std::copy(mixer_copy.begin(), mixer_copy.end(), dest);
}


template <size_t count, typename IntRep, size_t mix_rounds>
template <typename RandomAccessIterator>
void seed_seq_fe<count,IntRep,mix_rounds>::generate(
        RandomAccessIterator dest_begin,
        RandomAccessIterator dest_end) const
{
    auto src_begin = mixer_.begin();
    auto src_end   = mixer_.end();
    auto src       = src_begin;
    auto hash_const = INIT_B;
    for (auto dest = dest_begin; dest != dest_end; ++dest) {
        auto dataval = *src;
        if (++src == src_end)
            src = src_begin;
        dataval ^= hash_const;
        hash_const *= MULT_B;
        dataval *= hash_const;
        dataval ^= dataval >> XSHIFT;
        *dest = dataval;
    }
}

using seed_seq_fe128 = seed_seq_fe<4, uint32_t>;
using seed_seq_fe256 = seed_seq_fe<8, uint32_t>;


//////////////////////////////////////////////////////////////////////////////
//
// auto_seeded
//
//////////////////////////////////////////////////////////////////////////////

/*
 * randutils::auto_seeded
 *
 *   Extends a seed sequence class with a nondeterministic default constructor.
 *   Uses a variety of local sources of entropy to portably initialize any
 *   seed sequence to a good default state.
 *
 *   In normal use, it's accessed via one of the following type aliases, which
 *   use seed_seq_fe128 and seed_seq_fe256 above.
 *
 *       randutils::auto_seed_128
 *       randutils::auto_seed_256
 *
 *   It's discussed in detail at
 *       http://www.pcg-random.org/posts/simple-portable-cpp-seed-entropy.html
 *   and its motivation (why you can't just use std::random_device) here
 *       http://www.pcg-random.org/posts/cpps-random_device.html
 */

template <typename SeedSeq>
class auto_seeded : public SeedSeq {
    using default_seeds = std::array<uint32_t, 11>;

    template <typename T>
    static uint32_t crushto32(T value)
    {
        if (sizeof(T) <= 4)
            return uint32_t(value);
        else {
            uint64_t result = uint64_t(value);
            result *= 0xbc2ad017d719504d;
            return uint32_t(result ^ (result >> 32));
        }
    }

    template <typename T>
    static uint32_t hash(T&& value)
    {
        return crushto32(std::hash<typename std::remove_reference<
                                    typename std::remove_cv<T>::type>::type>{}(
                                     std::forward<T>(value)));
    }

    static constexpr uint32_t fnv(uint32_t hash, const char* pos)
    {
        return *pos == '\0' ? hash : fnv((hash * 16777619U) ^ *pos, pos+1);
    }

    default_seeds local_entropy()
    {
        // This is a constant that changes every time we compile the code
        constexpr uint32_t compile_stamp =
            fnv(2166136261U, __DATE__ __TIME__ __FILE__);

        // Some people think you shouldn't use the random device much because
        // on some platforms it could be expensive to call or "use up" vital
        // system-wide entropy, so we just call it once.
        static uint32_t random_int = std::random_device{}();

        // The heap can vary from run to run as well.
        void* malloc_addr = malloc(sizeof(int));
        free(malloc_addr);
        auto heap  = hash(malloc_addr);
        auto stack = hash(&malloc_addr);

        // Every call, we increment our random int.  We don't care about race
        // conditons.  The more, the merrier.
        random_int += 0xedf19156;

        // Classic seed, the time.  It ought to change, especially since
        // this is (hopefully) nanosecond resolution time.
        auto hitime = std::chrono::high_resolution_clock::now()
                        .time_since_epoch().count();

        // Address of the thing being initialized.  That can mean that
        // different seed sequences in different places in memory will be
        // different.  Even for the same object, it may vary from run to
        // run in systems with ASLR, such as OS X, but on Linux it might not
        // unless we compile with -fPIC -pic.
        auto self_data = hash(this);

        // The address of the time function.  It should hopefully be in
        // a system library that hopefully isn't always in the same place
        // (might not change until system is rebooted though)
        auto time_func = hash(&std::chrono::high_resolution_clock::now);

        // The address of the exit function.  It should hopefully be in
        // a system library that hopefully isn't always in the same place
        // (might not change until system is rebooted though).  Hopefully
        // it's in a different library from time_func.
        auto exit_func = hash(&_Exit);

        // The address of a local function.  That may be in a totally
        // different part of memory.  On OS X it'll vary from run to run thanks
        // to ASLR, on Linux it might not unless we compile with -fPIC -pic.
        // Need the cast because it's an overloaded
        // function and we need to pick the right one.
        auto self_func = hash(
            static_cast<uint32_t (*)(uint64_t)>(
                                &auto_seeded::crushto32));

        // Hash our thread id.  It seems to vary from run to run on OS X, not
        // so much on Linux.
        auto thread_id  = hash(std::this_thread::get_id());

        // Hash of the ID of a type.  May or may not vary, depending on
        // implementation.
        #if __cpp_rtti || __GXX_RTTI
        auto type_id   = crushto32(typeid(*this).hash_code());
        #else
        uint32_t type_id   = 0;
        #endif

        // Platform-specific entropy
        auto pid = crushto32(RANDUTILS_GETPID);
        auto cpu = crushto32(RANDUTILS_CPU_ENTROPY);

        return {{random_int, crushto32(hitime), stack, heap, self_data,
                 self_func, exit_func, thread_id, type_id, pid, cpu}};
    }


public:
    using SeedSeq::SeedSeq;

    using base_seed_seq = SeedSeq;

    const base_seed_seq& base() const
    {
        return *this;
    }

    base_seed_seq& base()
    {
        return *this;
    }

    auto_seeded(default_seeds seeds)
        : SeedSeq(seeds.begin(), seeds.end())
    {
        // Nothing else to do
    }

    auto_seeded()
        : auto_seeded(local_entropy())
    {
        // Nothing else to do
    }
};

using auto_seed_128 = auto_seeded<seed_seq_fe128>;
using auto_seed_256 = auto_seeded<seed_seq_fe256>;


//////////////////////////////////////////////////////////////////////////////
//
// uniform_distribution
//
//////////////////////////////////////////////////////////////////////////////

/*
 * This template typedef provides either
 *    - uniform_int_distribution, or
 *    - uniform_real_distribution
 * depending on the provided type
 */

template <typename Numeric>
using uniform_distribution = typename std::conditional<
            std::is_integral<Numeric>::value,
              std::uniform_int_distribution<Numeric>,
              std::uniform_real_distribution<Numeric> >::type;



//////////////////////////////////////////////////////////////////////////////
//
// random_generator
//
//////////////////////////////////////////////////////////////////////////////

/*
 * randutils::random_generator
 *
 *   An Easy-to-Use Random API
 *
 *   Provides all the power of C++11's random number facility in an easy-to
 *   use wrapper.
 *
 *   In normal use, it's accessed via one of the following type aliases, which
 *   also use auto_seed_256 by default
 *
 *       randutils::default_rng
 *       randutils::mt19937_rng
 *
 *   It's discussed in detail at
 *       http://www.pcg-random.org/posts/ease-of-use-without-loss-of-power.html
 */

template <typename RandomEngine = std::default_random_engine,
          typename DefaultSeedSeq = auto_seed_256>
class random_generator {
public:
    using engine_type       = RandomEngine;
    using default_seed_type = DefaultSeedSeq;
private:
    engine_type engine_;

    // This SFNAE evilness provides a mechanism to cast classes that aren't
    // themselves (technically) Seed Sequences but derive from a seed
    // sequence to be passed to functions that require actual Seed Squences.
    // To do so, the class should provide a the type base_seed_seq and a
    // base() member function.

    template <typename T>
    static constexpr bool has_base_seed_seq(typename T::base_seed_seq*)
    {
        return true;
    }

    template <typename T>
    static constexpr bool has_base_seed_seq(...)
    {
        return false;
    }


    template <typename SeedSeqBased>
    static auto seed_seq_cast(SeedSeqBased&& seq,
                               typename std::enable_if<
                                 has_base_seed_seq<SeedSeqBased>(0)>::type* = 0)
                                        -> decltype(seq.base())
    {
        return seq.base();
    }

    template <typename SeedSeq>
    static SeedSeq seed_seq_cast(SeedSeq&& seq,
                                   typename std::enable_if<
                                     !has_base_seed_seq<SeedSeq>(0)>::type* = 0)
    {
        return seq;
    }

public:
    template <typename Seeding = default_seed_type,
              typename... Params>
    random_generator(Seeding&& seeding = default_seed_type{})
        : engine_{seed_seq_cast(std::forward<Seeding>(seeding))}
    {
        // Nothing (else) to do
    }

    // Work around Clang DR777 bug in Clang 3.6 and earlier by adding a
    // redundant overload rather than mixing parameter packs and default
    // arguments.
    //     https://llvm.org/bugs/show_bug.cgi?id=23029
    template <typename Seeding,
              typename... Params>
    random_generator(Seeding&& seeding, Params&&... params)
        : engine_{seed_seq_cast(std::forward<Seeding>(seeding)),
                  std::forward<Params>(params)...}
    {
        // Nothing (else) to do
    }

    template <typename Seeding = default_seed_type,
              typename... Params>
    void seed(Seeding&& seeding = default_seed_type{})
    {
        engine_.seed(seed_seq_cast(seeding));
    }

    // Work around Clang DR777 bug in Clang 3.6 and earlier by adding a
    // redundant overload rather than mixing parameter packs and default
    // arguments.
    //     https://llvm.org/bugs/show_bug.cgi?id=23029
    template <typename Seeding,
              typename... Params>
    void seed(Seeding&& seeding, Params&&... params)
    {
        engine_.seed(seed_seq_cast(seeding), std::forward<Params>(params)...);
    }


    RandomEngine& engine()
    {
        return engine_;
    }

    template <typename ResultType,
              template <typename> class DistTmpl = std::normal_distribution,
              typename... Params>
    ResultType variate(Params&&... params)
    {
        DistTmpl<ResultType> dist(std::forward<Params>(params)...);

        return dist(engine_);
    }

    template <typename Numeric>
    Numeric uniform(Numeric lower, Numeric upper)
    {
        return variate<Numeric,uniform_distribution>(lower, upper);
    }

    template <template <typename> class DistTmpl = uniform_distribution,
              typename Iter,
              typename... Params>
    void generate(Iter first, Iter last, Params&&... params)
    {
        using result_type =
           typename std::remove_reference<decltype(*(first))>::type;

        DistTmpl<result_type> dist(std::forward<Params>(params)...);

        std::generate(first, last, [&]{ return dist(engine_); });
    }

    template <template <typename> class DistTmpl = uniform_distribution,
              typename Range,
              typename... Params>
    void generate(Range&& range, Params&&... params)
    {
        generate<DistTmpl>(std::begin(range), std::end(range),
                           std::forward<Params>(params)...);
    }

    template <typename Iter>
    void shuffle(Iter first, Iter last)
    {
        std::shuffle(first, last, engine_);
    }

    template <typename Range>
    void shuffle(Range&& range)
    {
        shuffle(std::begin(range), std::end(range));
    }


    template <typename Iter>
    Iter choose(Iter first, Iter last)
    {
        auto dist = std::distance(first, last);
        if (dist < 2)
            return first;
        using distance_type = decltype(dist);
        distance_type choice = uniform(distance_type(0), --dist);
        std::advance(first, choice);
        return first;
    }

    template <typename Range>
    auto choose(Range&& range) -> decltype(std::begin(range))
    {
        return choose(std::begin(range), std::end(range));
    }


    template <typename Range>
    auto pick(Range&& range) -> decltype(*std::begin(range))
    {
        return *choose(std::begin(range), std::end(range));
    }

    template <typename T>
    auto pick(std::initializer_list<T> range) -> decltype(*range.begin())
    {
        return *choose(range.begin(), range.end());
    }


    template <typename Size, typename Iter>
    Iter sample(Size to_go, Iter first, Iter last)
    {
        auto total = std::distance(first, last);
        using value_type = decltype(*first);

        return std::stable_partition(first, last,
             [&](const value_type&) {
                --total;
                using distance_type = decltype(total);
                distance_type zero{};
                if (uniform(zero, total) < to_go) {
                    --to_go;
                    return true;
                } else {
                    return false;
                }
             });
    }

    template <typename Size, typename Range>
    auto sample(Size to_go, Range&& range) -> decltype(std::begin(range))
    {
        return sample(to_go, std::begin(range), std::end(range));
    }
};

using default_rng = random_generator<std::default_random_engine>;
using mt19937_rng = random_generator<std::mt19937>;

}

#endif // RANDUTILS_HPP
