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

//! @mainpage API to HFST ospell WFST spell-checking
//!
//! The hfst-ospell API has several layers for different end-users. A suggested
//! starting point for new user is the @c ZHfstOspeller object, which reads an
//! automaton set from zipped hfst file with metadata and provides high level
//! access to it with generic spell-checking, correction and analysis functions.
//! Second level of access is the Speller object, which can be used to
//! construct spell-checker with two automata and traverse it and query
//! low-level properties. The Speller is constructed with two Transducer objects
//! which are the low-level access point to the automata with all the gory
//! details of transition tables and symbol translations, headers and such.

#ifndef HFST_OSPELL_ZHFSTOSPELLER_H_
#define HFST_OSPELL_ZHFSTOSPELLER_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstdint>
#include <map>

#include "ospell.h"
#include "hfst-ol.h"
#include "ZHfstOspellerXmlMetadata.h"

namespace hfst_ol
{
//! @brief ZHfstOspeller class holds one speller contained in one
//!        zhfst file.
//!        Ospeller can perform all basic writer tool functionality that
//!        is supporte by the automata in the zhfst archive.
class ZHfstOspeller
{
public:
    //! @brief create speller with default values for undefined
    //!        language.
    ZHfstOspeller();

    //! @brief construct speller from named file containing valid
    //!        zhfst archive.
    ZHfstOspeller(const std::string& filename);

    //! @brief construct speller from acceptor and error model files.
    ZHfstOspeller(const std::string& acceptorFn,
                  const std::string& errmodelFn);

    //! @brief destroy all automata used by the speller.
    ~ZHfstOspeller();

    //! @brief assign a speller-suggestor circumventing the ZHFST format
    void inject_speller(Speller* s);
    //! @brief set upper limit to priority queue when performing
    //         suggestions or analyses.
    void set_queue_limit(uint64_t limit);
    //! @brief set upper limit for weights
    void set_weight_limit(Weight limit);
    //! @brief set search beam
    void set_beam(Weight beam);
    //! @brief construct speller from named file containing valid
    //!        zhfst archive. Returns temp directory.
    std::string read_zhfst(const std::string& filename);

    void set_temporary_dir(const std::string& tempdir);

    //! @brief  check if the given word is spelled correctly
    bool spell(const std::string& wordform);
    //! @brief construct an ordered set of corrections for misspelled
    //!        word form.
    std::vector<StringWeightPair>
    suggest(const std::string& wordform);
    //! @brief analyse word form morphologically
    //! @param wordform   the string to analyse
    //! @param ask_sugger whether to use the spelling correction model
    //                    instead of the detection model
    std::vector<StringWeightPair>
    analyse(const std::string& wordform, bool ask_sugger=false);
    //! @brief construct an ordered set of corrections with analyses
    std::vector<StringPairWeightPair>
    suggest_analyses(const std::string& wordform);
    //! @brief hyphenate word form
    std::vector<StringWeightPair>
    hyphenate(const std::string& wordform);

    #if USE_CACHE
    //! @brief Clears the internal cache (to free up memory)
    void clear_suggestion_cache(void);
    #endif

    //! @brief get access to metadata read from XML.
    const ZHfstOspellerXmlMetadata& get_metadata() const;
    //! @brief create string representation of the speller for
    //!        programmer to debug
    std::string metadata_dump() const;

private:
    //! @brief file or path where the speller came from
    std::string filename_;
    //! @brief upper bound for suggestions generated and given
    uint64_t suggestions_maximum_;
    //! @brief upper bound for suggestion weight generated and given
    Weight maximum_weight_;
    //! @brief upper bound for search beam around best candidate
    Weight beam_;
    //! @brief whether automatons loaded yet can be used to check
    //!        spelling
    bool can_spell_;
    //! @brief whether automatons loaded yet can be used to correct
    //!        word forms
    bool can_correct_;
    //! @brief whether automatons loaded yet can be used to analyse
    //!        word forms
    bool can_analyse_;
    //! @brief whether automatons loaded yet can be used to hyphenate
    //!        word forms
    bool can_hyphenate_;
    //! @brief dictionaries loaded
    std::map<std::string, Transducer*> acceptors_;
    //! @brief error models loaded
    std::map<std::string, Transducer*> errmodels_;
    //! @brief pointer to current speller
    Speller* current_speller_;
    //! @brief pointer to current correction model
    Speller* current_sugger_;
    //! @brief pointer to current morphological analyser
    Speller* current_analyser_;
    //! @brief pointer to current hyphenator
    Transducer* current_hyphenator_;
    //! @brief the metadata of loaded speller
    ZHfstOspellerXmlMetadata metadata_;
    //! @brief temporary directory for files
    std::string tmp_prefix_;

    CorrectionQueue suggest_queue(const std::string& wordform);
    AnalysisQueue analyse_queue(const std::string& wordform, bool ask_sugger);
    Transducer* load_acceptor(struct archive* ar, struct archive_entry* entry, const char* filename, const std::string& tempdir);
    Transducer* load_errmodel(struct archive* ar, struct archive_entry* entry, const char* filename, const std::string& tempdir);
};

//! @brief Top-level exception for zhfst handling.

//! Contains a human-readable error message that can be displayed to
//! end-user as additional info when either solving exception or exiting.
class ZHfstException
{
public:
    ZHfstException();
    //! @brief construct error with human readable message.
    //!
    //! the message will be displayed when recovering or dying from
    //! exception
    explicit ZHfstException(const std::string& message);
    //!
    //! format error as user-readable message
    const char* what();
private:
    std::string what_;
};

//! @brief Generic error in metadata parsing.
//
//! Gets raised if metadata is erroneous or missing.
class ZHfstMetaDataParsingError : public ZHfstException
{
public:
    explicit ZHfstMetaDataParsingError(const std::string& message);
private:
    std::string what_;
};

//! @brief Exception for XML parser errors.
//
//! Gets raised if underlying XML parser finds an error in XML data.
//! Errors include non-valid XML, missing or erroneous attributes or
//! elements, etc.
class ZHfstXmlParsingError : public ZHfstException
{
public:
    explicit ZHfstXmlParsingError(const std::string& message);
private:
    std::string what_;
};

//! @brief Generic error while reading zip file.
//!
//! Happens when libarchive is unable to proceed reading zip file or
//! zip file is missing required files.
class ZHfstZipReadingError : public ZHfstException
{
public:
    explicit ZHfstZipReadingError(const std::string& message);
private:
    std::string what_;
};

//! @brief Error when writing to temporary location.
//
//! This exception gets thrown, when e.g., zip extraction is unable to
//! find or open temporary file for writing.
class ZHfstTemporaryWritingError : public ZHfstException
{
public:
    explicit ZHfstTemporaryWritingError(const std::string& message);
private:
    std::string what_;
};

}   // namespace hfst_ol


#endif // HFST_OSPELL_OSPELLER_SET_H_
// vim: set ft=cpp.doxygen:
