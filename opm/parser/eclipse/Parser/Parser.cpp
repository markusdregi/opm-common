/*
  Copyright 2013 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/RawDeck/RawEnums.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>

namespace Opm {

    Parser::Parser(bool addDefault) {
        if (addDefault)
            addDefaultKeywords();
    }

    DeckPtr Parser::parse(const std::string &dataFile) const {
        return parse(dataFile, true);
    }


    /*
     About INCLUDE: Observe that the ECLIPSE parser is slightly unlogical
     when it comes to nested includes; the path to an included file is always
     interpreted relative to the filesystem location of the DATA file, and
     not the location of the file issuing the INCLUDE command. That behaviour
     is retained in the current implementation.
     */

    DeckPtr Parser::parse(const std::string &dataFileName, bool strictParsing) const {

        std::shared_ptr<ParserState> parserState(new ParserState());
        parserState->strictParsing = strictParsing;
        parserState->dataFile = boost::filesystem::path(dataFileName);;
        if (parserState->dataFile.is_absolute())
            parserState->rootPath = parserState->dataFile.parent_path();
        else
            parserState->rootPath = boost::filesystem::current_path() / parserState->dataFile.parent_path();

        parserState->deck = DeckPtr(new Deck());

        parseFile(parserState);
        return parserState->deck;
    }


    size_t Parser::size() const {
        return m_parserKeywords.size();
    }
    
    
    void Parser::addKeyword(ParserKeywordConstPtr parserKeyword) {
        const std::string& name = parserKeyword->getName();
        dropKeyword( name );
        
        m_parserKeywords.insert(std::make_pair(name, parserKeyword));
        if (ParserKeyword::wildCardName(name)) 
            m_wildCardKeywords.insert( std::make_pair(name , parserKeyword ));
    }


    ParserKeywordConstPtr Parser::matchingKeyword(const std::string& name) const {
        std::map<std::string, ParserKeywordConstPtr>::const_iterator iter = m_wildCardKeywords.begin();
        ParserKeywordConstPtr keyword;
        
        while (true) {
            if (iter == m_wildCardKeywords.end())
                break;

            if ((*iter).second->matches(name)) {
                keyword = (*iter).second;
                break;
            }

            ++iter;
        }
        return keyword;
    }


    bool Parser::hasKeyword(const std::string& keyword) const {
        return (m_parserKeywords.find(keyword) != m_parserKeywords.end());
    }


    bool Parser::hasWildCardKeyword(const std::string& keyword) const {
        return (m_wildCardKeywords.find(keyword) != m_parserKeywords.end());
    }
    

    bool Parser::canParseKeyword( const std::string& keyword) const {
        if (hasKeyword(keyword)) 
            return true;
        else {
            ParserKeywordConstPtr wildCardKeyword = matchingKeyword( keyword );
            if (wildCardKeyword)
                return true;
            else
                return false;
        }
    }



    bool Parser::dropKeyword(const std::string& keyword) {
        bool erase = (m_parserKeywords.erase( keyword ) == 1);
        if (erase)
            m_wildCardKeywords.erase( keyword );
        
        return erase;
    }



    ParserKeywordConstPtr Parser::getKeyword(const std::string& keyword) const {
        if (hasKeyword(keyword)) {
            return m_parserKeywords.at(keyword);
        } else {
            ParserKeywordConstPtr wildCardKeyword = matchingKeyword( keyword );

            if (wildCardKeyword)
                return wildCardKeyword;
            else
                throw std::invalid_argument("Do not have parser keyword for parsing: " + keyword);
        }
    }



    void Parser::parseFile(std::shared_ptr<ParserState> parserState) const {
        bool verbose = false;
        parserState->lineNR = 0;
        parserState->inputstream.open(parserState->dataFile.string().c_str());

        if (parserState->inputstream) {
            while (true) {
                bool streamOK = tryParseKeyword(parserState);
                if (parserState->rawKeyword) {
                    if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::include) {
                        RawRecordConstPtr firstRecord = parserState->rawKeyword->getRecord(0);
                        std::string includeFileString = firstRecord->getItem(0);
                        boost::filesystem::path includeFile(includeFileString);
                        
                        if (includeFile.is_relative())
                            includeFile = parserState->rootPath / includeFile;
                        
                        if (verbose)
                            std::cout << parserState->rawKeyword->getKeywordName() << "  " << includeFile << std::endl;
                        

                        std::shared_ptr<ParserState> newParserState (new ParserState());
                        newParserState->copyFrom(parserState);
                        newParserState->dataFile = includeFile;
                        parseFile(newParserState);
                    } else {
                        if (verbose)
                            std::cout << parserState->rawKeyword->getKeywordName() << std::endl;
                        
                        if (canParseKeyword(parserState->rawKeyword->getKeywordName())) {
                            ParserKeywordConstPtr parserKeyword = getKeyword(parserState->rawKeyword->getKeywordName());
                            ParserKeywordActionEnum action = parserKeyword->getAction();
                            if (action == INTERNALIZE) {
                                DeckKeywordPtr deckKeyword = parserKeyword->parse(parserState->rawKeyword);
                                parserState->deck->addKeyword(deckKeyword);
                            } else if (action == IGNORE_WARNING) 
                                parserState->deck->addWarning( "The keyword " + parserState->rawKeyword->getKeywordName() + " is ignored - this might potentially affect the results" , parserState->dataFile.string() , parserState->rawKeyword->getLineNR());
                        } else {
                            DeckKeywordPtr deckKeyword(new DeckKeyword(parserState->rawKeyword->getKeywordName(), false));
                            parserState->deck->addKeyword(deckKeyword);
                            parserState->deck->addWarning( "The keyword " + parserState->rawKeyword->getKeywordName() + " is not recognized" , parserState->dataFile.string() , parserState->lineNR);
                        }
                    }
                    parserState->rawKeyword.reset();
                }
                if (!streamOK)
                    break;
            }
            
            parserState->inputstream.close();
        } else
            throw std::invalid_argument("Failed to open file: " + parserState->dataFile.string());
    }


    void Parser::loadKeywords(const Json::JsonObject& jsonKeywords) {
        if (jsonKeywords.is_array()) {
            for (size_t index = 0; index < jsonKeywords.size(); index++) {
                Json::JsonObject jsonKeyword = jsonKeywords.get_array_item(index);
                ParserKeywordConstPtr parserKeyword(new ParserKeyword(jsonKeyword));

                addKeyword(parserKeyword);
            }
        } else
            throw std::invalid_argument("Input JSON object is not an array");
    }

    RawKeywordPtr Parser::createRawKeyword(const std::string & keywordString, std::shared_ptr<ParserState> parserState) const {
        if (canParseKeyword(keywordString)) {
            ParserKeywordConstPtr parserKeyword = getKeyword( keywordString );
            ParserKeywordActionEnum action = parserKeyword->getAction();
            
            if (action == THROW_EXCEPTION)
                throw std::invalid_argument("Parsing terminated by fatal keyword: " + keywordString);
            
            if (parserKeyword->getSizeType() == SLASH_TERMINATED || parserKeyword->getSizeType() == UNKNOWN) {
                Raw::KeywordSizeEnum rawSizeType;
                if (parserKeyword->getSizeType() == SLASH_TERMINATED)
                    rawSizeType = Raw::SLASH_TERMINATED;
                else
                    rawSizeType = Raw::UNKNOWN;

                return RawKeywordPtr(new RawKeyword(keywordString , rawSizeType , parserState->dataFile.string(), parserState->lineNR));
            } else {
                size_t targetSize;

                if (parserKeyword->hasFixedSize())
                    targetSize = parserKeyword->getFixedSize();
                else {
                    const std::pair<std::string, std::string> sizeKeyword = parserKeyword->getSizeDefinitionPair();
                    DeckKeywordConstPtr sizeDefinitionKeyword = parserState->deck->getKeyword(sizeKeyword.first);
                    DeckItemConstPtr sizeDefinitionItem;
                    {
                        DeckRecordConstPtr record = sizeDefinitionKeyword->getRecord(0);
                        sizeDefinitionItem = record->getItem(sizeKeyword.second);
                    }
                    targetSize = sizeDefinitionItem->getInt(0);
                }
                return RawKeywordPtr(new RawKeyword(keywordString, parserState->dataFile.string() , parserState->lineNR , targetSize , parserKeyword->isTableCollection()));
            }
        } else {
            if (parserState->strictParsing) {
                throw std::invalid_argument("Keyword " + keywordString + " not recognized ");
            } else {
                return RawKeywordPtr(new RawKeyword(keywordString, parserState->dataFile.string(), parserState->lineNR , 0));
            }
        }
    }


    bool Parser::tryParseKeyword(std::shared_ptr<ParserState> parserState) const {
        std::string line;

        if (parserState->nextKeyword.length() > 0) {
            parserState->rawKeyword = createRawKeyword(parserState->nextKeyword, parserState);
            parserState->nextKeyword = "";
        }

        while (std::getline(parserState->inputstream, line)) {
            boost::algorithm::trim_right(line); // Removing garbage (eg. \r)
            std::string keywordString;
            parserState->lineNR++;
            if (parserState->rawKeyword == NULL) {
                if (RawKeyword::tryParseKeyword(line, keywordString)) {
                    parserState->rawKeyword = createRawKeyword(keywordString, parserState);
                }
            } else {
                if (parserState->rawKeyword->getSizeType() == Raw::UNKNOWN) {
                    if (canParseKeyword(line)) {
                        parserState->nextKeyword = line;
                        return true;
                    }
                }
                if (RawKeyword::useLine(line)) {
                    parserState->rawKeyword->addRawRecordString(line);
                }
            }

            if (parserState->rawKeyword != NULL && parserState->rawKeyword->isFinished())
                return true;
        }

        return false;
    }

    bool Parser::loadKeywordFromFile(const boost::filesystem::path& configFile) {

        try {
            Json::JsonObject jsonKeyword(configFile);
            ParserKeywordConstPtr parserKeyword(new ParserKeyword(jsonKeyword));
            addKeyword(parserKeyword);
            return true;
        } 
        catch (...) {
            return false;
        }
    }


    void Parser::loadKeywordsFromDirectory(const boost::filesystem::path& directory, bool recursive, bool onlyALLCAPS8) {
        if (!boost::filesystem::exists(directory))
            throw std::invalid_argument("Directory: " + directory.string() + " does not exist.");
        else {
            boost::filesystem::directory_iterator end;
            for (boost::filesystem::directory_iterator iter(directory); iter != end; iter++) {
                if (boost::filesystem::is_directory(*iter)) {
                    if (recursive)
                        loadKeywordsFromDirectory(*iter, recursive, onlyALLCAPS8);
                } else {
                    if (ParserKeyword::validName(iter->path().filename().string()) || !onlyALLCAPS8) {
                        if (!loadKeywordFromFile(*iter))
                            std::cerr << "** Warning: failed to load keyword from file:" << iter->path() << std::endl;
                    }
                }
            }
        }
    }



} // namespace Opm
