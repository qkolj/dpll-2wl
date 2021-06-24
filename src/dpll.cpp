#include "dpll.h"

#include <string>
#include <sstream>
#include <stdexcept>
#include <iterator>
#include <algorithm>

#ifdef DEBUG
#define DB(X) X
#else
#define DB(X)
#endif

DPLL::DPLL(const CNFFormula &formula)
: _formula(formula), _valuation(_formula.size()), _literals_watchlist(), _clauses_watched_literals()
{
    initWatchlists();
}

DPLL::DPLL(std::istream &dimacsStream)
{
    std::string line;
    std::size_t firstNonSpaceIndex = 0;
    std::string error_message = "Incorrect DIMACS stream";

    while(std::getline(dimacsStream, line))
    {
        firstNonSpaceIndex = line.find_first_not_of(" \t\r\n");
        if(firstNonSpaceIndex != std::string::npos && line[firstNonSpaceIndex] != 'c')
            break;
    }
    
    if(line[firstNonSpaceIndex] != 'p')
        throw std::runtime_error(error_message);

    std::istringstream parser{line.substr(firstNonSpaceIndex + 1, std::string::npos)};
    std::string tmp;

    if(!(parser >> tmp) || tmp != "cnf")
        throw std::runtime_error(error_message);

    unsigned varCount, claCount;

    if(!(parser >> varCount >> claCount))
        throw std::runtime_error(error_message);
    
    _formula.resize(claCount);
    _valuation.reset(varCount);
    int clauseIndex = 0;

    while(std::getline(dimacsStream, line))
    {
        firstNonSpaceIndex = line.find_first_not_of(" \t\r\n");

        if(firstNonSpaceIndex != std::string::npos && line[firstNonSpaceIndex] != 'c')
        {
            parser.clear();
            parser.str(line);
            std::copy(std::istream_iterator<int>{parser}, {}, std::back_inserter(_formula[clauseIndex]));
            _formula[clauseIndex++].pop_back();
        }
    }

    initWatchlists();
}

void DPLL::initWatchlists()
{
    for(const Clause &c : _formula)
        if(c.size() >= 2)
        {
            if(_clauses_watched_literals[c].empty())
            {
                if(c[0] != c[1])
                {
                    _clauses_watched_literals[c].push_back(c[0]);
                    _literals_watchlist[c[0]].push_back(c);
                    _clauses_watched_literals[c].push_back(c[1]);
                    _literals_watchlist[c[1]].push_back(c);
                }
                else
                {
                    auto i = c.begin();

                    while(i != c.end() && (*i) != c[0])
                        i++;

                    if(i != c.end())
                    {
                        _clauses_watched_literals[c].push_back(*i);
                        _literals_watchlist[*i].push_back(c);
                    }
                    else
                    {
                        _added_unit.push_back(c[0]);
                    }
                }
            }
        }
        else
        {
            _added_unit.push_back(c[0]);
        }
}

OptionalPartialValuation DPLL::solve()
{
    bool conflict = false;
    std::vector<Literal> unitPropQueue;
    
    DB(std::cout << "UNIT CLAUSES FROM THE START: ";
    for(const auto l : _added_unit)
        std::cout << l << " ";
    std::cout << std::endl;);
    
    for(const auto l : _added_unit)
    {
        DB(std::cout << "UNIT PROPING " << l << std::endl);
        _valuation.push(l);

        DB(std::cout << "UPDATING WATCHLISTS FOR " << -l << std::endl);
        if(updateWatchlists(-l, &unitPropQueue))
        {
            DB(std::cout << "CONFLICT WHILE PROCESSING CLAUSES WITH ONLY ONE LITERAL, FORMULA IS UNSATISFIABLE");
            return {};
        }
    }
    
    while(unitPropQueue.size() > 0)
    {
        DB(std::cout << "UNIT PROP QUEUE IS ";
        for(const auto a : unitPropQueue)
            std::cout << a << " ";
        std::cout << std::endl);

        Literal l = unitPropQueue.back();
        unitPropQueue.pop_back();
        _valuation.push(l);
        if(updateWatchlists(-l, &unitPropQueue))
        {
            DB(std::cout << "CONFLICT WHILE PROCESSING CLAUSES WITH ONLY ONE LITERAL, FORMULA IS UNSATISFIABLE");
            return {};
        }
    }
    
    while(true)
    {
        DB(std::cout << _valuation << std::endl);

        if(unitPropQueue.size() > 0 && !conflict)
        {
            DB(std::cout << "UNIT PROP QUEUE IS ";
            for(const auto a : unitPropQueue)
                std::cout << a << " ";
            std::cout << std::endl);

            while(!conflict && unitPropQueue.size() > 0)
            {
                Literal l = unitPropQueue.back();
                unitPropQueue.pop_back();

                if(_valuation.isLiteralUndefined(l))
                {
                    DB(std::cout << "UNIT PROPING " << l << std::endl);
                    _valuation.push(l);

                    DB(std::cout << "UPDATING WATCHLISTS FOR " << -l << std::endl);
                    conflict = updateWatchlists(-l, &unitPropQueue);
                    if(conflict)
                        break;
                }
                else
                {
                    if((l > 0 && _valuation.isLiteralTrue(l)) || (l < 0 && _valuation.isLiteralFalse(l)))
                    {
                        DB(std::cout << "CANNOT UNIT PROP " << l << " REPORTING CONFLICT" << std::endl);
                        conflict = true;
                        break;
                    }
                }
            }
        }
        
        if(conflict)
        {
            unitPropQueue.clear();

            DB(std::cout << "BACKTRACKING" << std::endl);
            Literal decidedLiteral = _valuation.backtrack();

            if(decidedLiteral == NullLiteral)
                return {};
            
            DB(std::cout << "PUSHING " << -decidedLiteral << std::endl);
            _valuation.push(-decidedLiteral);

            DB(std::cout << "UPDATING WATCHLISTS FOR " << decidedLiteral << std::endl);
            conflict = updateWatchlists(decidedLiteral, &unitPropQueue);
        }
        else
        {
            Literal l;

            if((l = _valuation.firstUndefined()))
            {
                DB(std::cout << "DECIDING " << l << std::endl);
                _valuation.push(l, true);

                DB(std::cout << "UPDATING WATCHLISTS FOR " << -l << std::endl);
                conflict = updateWatchlists(-l, &unitPropQueue);
            }
            else
            {
                return _valuation;
            }
        }
    }
}

bool DPLL::updateWatchlists(Literal l, std::vector<Literal>* unitPropQueue)
{
    std::vector<Clause> unwatch;

    for(auto cl = _literals_watchlist[l].begin(); cl != _literals_watchlist[l].end(); cl++)
    {
        Clause c = *cl;
        bool foundUndefined = false, foundTrue = false;
        Literal undef;

        DB(std::cout << "CHECKING CLAUSE ";
        for(const auto li : c)
            std::cout << li << "(" << (_valuation.isLiteralTrue(li) ? 1 : (_valuation.isLiteralUndefined(li) ? 0 : -1)) << ") ");

        for(const Literal lit : c)
        {
            if(_valuation.isLiteralTrue(lit))
            {
                foundTrue = true;
                break;
            }
            else if(!foundUndefined && _valuation.isLiteralUndefined(lit) && _clauses_watched_literals[c][0] != lit && _clauses_watched_literals[c][1] != lit)
            {
                foundUndefined = true;
                undef = lit;
            }
        }

        DB(std::cout << std::endl << "IN CLAUSE ";
            for(const auto li : c)
                std::cout << li << " ");

        if(foundTrue)
        {
            DB(std::cout << "FOUND TRUE" << std::endl);
            continue;
        }
        else if(foundUndefined)
        {
            DB(std::cout << "FOUND UNDEFINED, SWITCHING WATCHED LITERAL TO " << undef << std::endl);

            unsigned index = _clauses_watched_literals[c][0] == l ? 0 : 1;

            _clauses_watched_literals[c][index] = undef;
            _literals_watchlist[undef].push_back(c);
            unwatch.push_back(c);
        }
        else
        {
            unsigned otherIndex = _clauses_watched_literals[c][0] == l ? 1 : 0;

            if(_valuation.isLiteralUndefined(_clauses_watched_literals[c][otherIndex]))
            {
                auto tmp = std::find(unitPropQueue->begin(), unitPropQueue->end(), -_clauses_watched_literals[c][otherIndex]);

                if(tmp == unitPropQueue->end())
                {
                    DB(std::cout << "ADDING " << _clauses_watched_literals[c][otherIndex] << " TO UNIT PROP QUEUE" << std::endl);
                    unitPropQueue->push_back(_clauses_watched_literals[c][otherIndex]);
                }
                else
                {
                    DB(std::cout << "OPOSITE LITERAL OF " << _clauses_watched_literals[c][otherIndex] << " ALREADY IN VALUATION, REPORTING CONFLICT" << std::endl);
                    return true;
                }
            }
            else
            {
                DB(std::cout << "ALL FALSE, REPORTING CONFLICT" << std::endl);
                return true;
            }
        }
    }
    
    for(const auto &c : unwatch)
        _literals_watchlist[l].erase(std::remove(_literals_watchlist[l].begin(), _literals_watchlist[l].end(), c), _literals_watchlist[l].end());
    
    return false;
}
