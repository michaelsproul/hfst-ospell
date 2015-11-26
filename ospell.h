/* -*- Mode: C++ -*- */
// Copyright 2010 University of Helsinki
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#ifndef HFST_OSPELL_OSPELL_H_
#define HFST_OSPELL_OSPELL_H_ 1

#include <string>
#include <deque>
#include <queue>
#include <list>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include "hfst-ol.h"

namespace hfst_ol {

//! Internal class for transition processing.

struct TreeNode;
struct CacheContainer;
typedef std::pair<std::string, std::string> StringPair;
typedef std::pair<std::string, Weight> StringWeightPair;
typedef std::vector<StringWeightPair> StringWeightVector;
typedef std::pair<std::pair<std::string, std::string>, Weight>
    StringPairWeightPair;
typedef std::vector<TreeNode> TreeNodeVector;
typedef std::map<std::string, Weight> StringWeightMap;

//! Contains low-level processing stuff.
struct STransition
{
    TransitionTableIndex index; //!< index to transition
    SymbolNumber symbol; //!< symbol of transition
    Weight weight; //!< weight of transition

    //!
    //! create transition without weight
    STransition(TransitionTableIndex i,
                SymbolNumber s) :
        index(i),
        symbol(s),
        weight(0.0)
    {
    }

    //! create transition with weight
    STransition(TransitionTableIndex i,
                SymbolNumber s,
                Weight w) :
        index(i),
        symbol(s),
        weight(w)
    {
    }

};
//! @brief comparison for establishing order for priority queue for suggestions.

//! The suggestions that are stored in a priority queue are arranged in
//! ascending order of the weight component, following the basic penalty
//! weight logic of tropical semiring that is present in most weighted
//! finite-state spell-checking automata.
class StringWeightComparison
    /* results are reversed by default because greater weights represent
       worse results - to reverse the reversal, give a true argument*/

{
    bool reverse;
public:
    //!
    //! construct a result comparator with ascending or descending weight order
    StringWeightComparison(bool reverse_result=false) :
        reverse(reverse_result)
    {
    }

    //!
    //! compare two string weight pairs for weights
    bool operator() (StringWeightPair lhs, StringWeightPair rhs);
};

//! @brief comparison for complex analysis queues
//
//! Follows weight value logic.
//! @see StringWeightComparison.
class StringPairWeightComparison
{
protected:
    bool reverse;
public:
    //!
    //! create result comparator with ascending or descending weight order
    StringPairWeightComparison(bool reverse_result=false) :
        reverse(reverse_result)
    {
    }

    //!
    //! compare two analysis corrections for weights
    bool operator() (StringPairWeightPair lhs, StringPairWeightPair rhs);
};

template<
    class T,
    class Container = std::vector<T>,
    class Compare = std::less<typename Container::value_type>
    >
class priority_queue : public std::priority_queue<T, Container, Compare>
{
public:
    Container clone_container()
    {
        Container container(this->c);
        std::sort(container.rbegin(), container.rend(), this->comp);
        return container;
    }
};

template<
    class T,
    class Container = std::vector<T>,
    class Compare = std::less<typename Container::value_type>
    >
class sized_priority_deque : public priority_queue<T, Container, Compare>
{
protected:
    size_t max_size;
public:
    explicit sized_priority_deque(size_t ms) :
        max_size(ms)
    {
    }
    explicit sized_priority_deque() :
        max_size(0)
    {
    }
    T bottom(void) const
    {
        return this->c.back();
    }
    void push(const T& value)
    {
        std::priority_queue<T, Container, Compare>::push(value);
        if (this->max_size > 0 && this->size() > max_size)
        {
            this->pop();
        }
    }
    void push_back(const T& value)
    {
        std::priority_queue<T, Container, Compare>::push(value);
        if (this->max_size > 0 && this->size() > max_size)
        {
            this->c.pop_back();
        }
    }
};

typedef priority_queue<StringWeightPair,
                            std::vector<StringWeightPair>,
                            StringWeightComparison> CorrectionQueue;
typedef priority_queue<StringWeightPair,
                            std::vector<StringWeightPair>,
                            StringWeightComparison> AnalysisQueue;
typedef priority_queue<StringWeightPair,
                            std::vector<StringWeightPair>,
                            StringWeightComparison> HyphenationQueue;
typedef priority_queue<StringPairWeightPair,
                            std::vector<StringPairWeightPair>,
                            StringPairWeightComparison> AnalysisCorrectionQueue;

class WeightQueue : public std::list<Weight>
{
private:
    size_t max_size;
    void prune();
public:
    WeightQueue() : max_size(0)
    {
    }
    WeightQueue(size_t sz) : max_size(sz)
    {
    }
    void push(Weight w); // add a new weight
    void pop(void); // delete the biggest weight
    Weight get_lowest(void) const;
    Weight get_highest(void) const;
};

//! Internal class for Transducer processing.

//! Contains low-level processing stuff.
class Transducer
{
private:
    int8_t* raw_ = nullptr;
    size_t len_ = 0;
protected:
    TransducerHeader header; //!< header data
    TransducerAlphabet alphabet; //!< alphabet data
    KeyTable * keys; //!< key symbol mappings
    Encoder encoder; //!< encoder to convert the strings

    static const TransitionTableIndex START_INDEX = 0; //!< position of first
public:
    //!
    //! read transducer from file @a f
    static Transducer from_file(std::string& filename);
    static Transducer* new_from_file(std::string& filename);
    //!
    //! read transducer from raw data @a data
    Transducer(int8_t * raw);
    ~Transducer();
    IndexTable indices; //!< index table
    TransitionTable transitions; //!< transition table
    //!
    //! Deprecated functions for single-tranducer lookup
    //! Speller::analyse() is recommended
    bool initialize_input_vector(SymbolVector & input_vector,
                                 Encoder * encoder,
                                 int8_t * line);
    AnalysisQueue lookup(int8_t * line);
    //!
    //! whether it's final transition in this transducer
    bool final_transition(TransitionTableIndex i);
    //!
    //! whether it's final index
    bool final_index(TransitionTableIndex i);
    //!
    //! get transducers symbol table mapping
    KeyTable * get_key_table(void);
    //!
    //! find key for string or create it
    SymbolNumber find_next_key(int8_t ** p);
    //!
    //! get encoder for mapping sttrings and symbols
    Encoder * get_encoder(void);
    //!
    //! get size of a state
    uint32_t get_state_size(void);
    //!
    //! get position of the ? symbols
    SymbolNumber get_unknown(void) const;
    SymbolNumber get_identity(void) const;
    //!
    //! get alphabet of automaton
    TransducerAlphabet * get_alphabet(void);
    //!
    //! get flag stuff of automaton
    OperationMap * get_operations(void);
    //!
    //! follow epsilon transitions from index
    STransition take_epsilons(const TransitionTableIndex i) const;
    //!
    //! follow epsilon transitions and falsg form index
    STransition take_epsilons_and_flags(const TransitionTableIndex i);
    //!
    //! follow real transitions from index
    STransition take_non_epsilons(const TransitionTableIndex i,
                                  const SymbolNumber symbol) const;
    //!
    //! get next index
    TransitionTableIndex next(const TransitionTableIndex i,
                              const SymbolNumber symbol) const;
    //!
    //! get next epsilon inedx
    TransitionTableIndex next_e(const TransitionTableIndex i) const;
    //!
    //! whether state has any transitions with @a symbol
    bool has_transitions(const TransitionTableIndex i,
                         const SymbolNumber symbol) const;
    //!
    //! whether state has epsilon s or flag s
    bool has_epsilons_or_flags(const TransitionTableIndex i);
    //!
    //! whether state has non-epsilons or non-flags
    bool has_non_epsilons_or_flags(const TransitionTableIndex i);
    //!
    //! whether it's final
    bool is_final(const TransitionTableIndex i);
    //!
    //! get final weight
    Weight final_weight(const TransitionTableIndex i) const;
    //!
    //! whether it's a flag
    bool is_flag(const SymbolNumber symbol);
    //!
    //! whether it's weighedc
    bool is_weighted(void);

};

//! Internal class for alphabet processing.

//! Contains low-level processing stuff.
struct TreeNode
{
    //    SymbolVector input_string; //<! the current input vector
    SymbolVector string; //!< the current output vector
    uint32_t input_state; //!< its input state
    TransitionTableIndex mutator_state; //!< state in error model
    TransitionTableIndex lexicon_state; //!< state in language model
    FlagDiacriticState flag_state; //!< state of flags
    Weight weight; //!< weight

    //!
    //! construct a node in trie from all that stuff
    TreeNode(SymbolVector prev_string,
             uint32_t i,
             TransitionTableIndex mutator,
             TransitionTableIndex lexicon,
             FlagDiacriticState state,
             Weight w) :
        string(prev_string),
        input_state(i),
        mutator_state(mutator),
        lexicon_state(lexicon),
        flag_state(state),
        weight(w)
    {
    }

    //!
    //! construct empty node with a starting state for flags
    TreeNode(FlagDiacriticState start_state) : // starting state node
        string(SymbolVector()),
        input_state(0),
        mutator_state(0),
        lexicon_state(0),
        flag_state(start_state),
        weight(0.0)
    {
    }

    //!
    //! check if tree node is compatible with flag diacritc
    bool try_compatible_with(FlagDiacriticOperation op);

    //!
    //! traverse some node in lexicon
    TreeNode update_lexicon(SymbolNumber next_symbol,
                            TransitionTableIndex next_lexicon,
                            Weight weight);

    //!
    //! traverse some node in error model
    TreeNode update_mutator(TransitionTableIndex next_mutator,
                            Weight weight);

    //!
    //! The update functions return updated copies of this state
    TreeNode update(SymbolNumber output_symbol,
                    uint32_t next_input,
                    TransitionTableIndex next_mutator,
                    TransitionTableIndex next_lexicon,
                    Weight weight);

    TreeNode update(SymbolNumber output_symbol,
                    TransitionTableIndex next_mutator,
                    TransitionTableIndex next_lexicon,
                    Weight weight);


};

typedef std::vector<TreeNode> TreeNodeQueue;

int nByte_utf8(uint8_t c);

//! Exception when speller cannot map characters of error model to language
//! model.

//! May get raised if error model automaton has output characters that are not
//! present in language model.
class AlphabetTranslationException : public std::runtime_error
{ // "what" should hold the first untranslatable symbol
public:

    //!
    //! create alpabet exception with symbol as explanation
    AlphabetTranslationException(const std::string what) :
        std::runtime_error(what)
    {
    }
};

//! @brief Basic spell-checking automata pair unit.

//! Speller consists of two automata, one for language modeling and one for
//! error modeling. The speller object has low-level access to the automata
//! and convenience functions for checking, analysing and correction.
//! @see ZHfstOspeller for high level access.
class Speller
{
protected:
    std::map<std::string, Weight> generate_correction_map(int32_t nbest, Weight maxweight, Weight beam);
    void set_limiting_behaviour(int32_t nbest, Weight maxweight, Weight beam);
    bool is_under_weight_limit(Weight w) const;
    void adjust_weight_limits(int32_t nbest, Weight beam);
    void lexicon_consume(void);
    //!
    //! traverse epsilons in error model
    void mutator_epsilons(void);
    //! traverse along input
    void consume_input();
    //! @brief Construct a cache entry for @a first_sym..
    void build_cache(SymbolNumber first_sym);
    //! size of states
    SymbolNumber get_state_size(void);
    //!
    //! initialise string conversions
    void build_alphabet_translator(void);
    void add_symbol_to_alphabet_translator(SymbolNumber to_sym);
    //!
    //! initialize input string
    bool init_input(int8_t * line);
    //!
    //! travers epsilons in language model
    void lexicon_epsilons(void);
    bool has_lexicon_epsilons(void) const
    {
        return lexicon->has_epsilons_or_flags(next_node.lexicon_state + 1);
    }
    bool has_mutator_epsilons(void) const
    {
        return mutator->has_transitions(next_node.mutator_state + 1, 0);
    }
    //!
    //! helper functions for traversal
    void queue_mutator_arcs(SymbolNumber input);
    void queue_lexicon_arcs(SymbolNumber input,
                            uint32_t mutator_state,
                            Weight mutator_weight = 0.0,
                            int input_increment = 0);
    CorrectionQueue handle_input_size_lt_1(SymbolNumber first_input, int32_t nbest, Weight beam);
public:
    Transducer * mutator; //!< error model
    Transducer * lexicon; //!< languag model
    SymbolVector input; //!< current input
    TreeNodeQueue node_queue; //!< current traversal fifo stack
    TreeNode next_node;  //!< current next node
    Weight limit; //!< current limit for weights
    Weight best_suggestion; //!< best suggestion so far
    WeightQueue nbest_queue; //!< queue to keep track of current n best results
    SymbolVector alphabet_translator; //!< alphabets in automata
    OperationMap * operations; //!< flags in it
    //!< A cache for the result of first symbols
    std::vector<CacheContainer> cache;
    //!< what kind of limiting behaviour we have
    enum LimitingBehaviour
    {
        None = 0,
        MaxWeight = (1u << 0),
        Nbest = (1u << 1),
        Beam = (1u << 2),
        MaxWeightNbest = MaxWeight | Nbest,
        MaxWeightBeam = MaxWeight | Beam,
        NbestBeam = Nbest | Beam,
        MaxWeightNbestBeam = MaxWeight | Nbest | Beam
    } limiting;
    //! what mode we're in
    enum Mode { Check, Correct, Lookup } mode;

    //!
    //! Create a speller object form error model and language automata.
    Speller(Transducer * mutator_ptr, Transducer * lexicon_ptr);

    //! @brief Check if the given string is accepted by the speller
    bool check(int8_t * line);
    //! @brief suggest corrections for given string @a line.
    //
    //! The number of corrections given and stored at any given time
    //! is limited by @a nbest if ≥ 0.
    CorrectionQueue correct(int8_t * line, int32_t nbest = 0,
                            Weight maxweight = -1.0,
                            Weight beam = -1.0);

    //! @brief analyse given string @a line.
    //
    //! If language model is two-tape, give a list of analyses for string.
    //! If not, this should return queue of one result @a line if the
    //! string is in language model and 0 results if it isn't.
    AnalysisQueue analyse(int8_t * line, int32_t nbest = 0);

};

struct CacheContainer
{
    // All the nodes that ultimately result from searching at input depth 1
    TreeNodeVector nodes;
    // The results are for length max one inputs only
    StringWeightVector results_len_0;
    StringWeightVector results_len_1;
    bool empty;

    CacheContainer(void) : empty(true)
    {
    }

    void clear(void)
    {
        nodes.clear();
        results_len_0.clear();
        results_len_1.clear();
    }

    inline StringWeightVector& get(int8_t sz)
    {
        if (sz == 0)
        {
            return results_len_0;
        }
        else
        {
            return results_len_1;
        }
    }
};

std::string stringify(KeyTable * key_table,
                      SymbolVector & symbol_vector);

} // namespace hfst_ol

// Some platforms lack strndup
char* hfst_strndup(const char* s, size_t n);

#endif // HFST_OSPELL_OSPELL_H_
