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
    std::size_t firstNonSpaceIndex;
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

OptionalPartialValuation DPLL::solve()
{
    while(true)
    {
//         std::cout << _valuation << std::endl;
        Literal l;
        if(hasConflict())
        {
            Literal decidedLiteral = _valuation.backtrack();
            if(decidedLiteral == NullLiteral)
                return {};
            
            _valuation.push(-decidedLiteral);
        }
        else if((l = hasUnitClause()))
        {
            _valuation.push(l);
        }
        else
        {
            if((l = _valuation.firstUndefined()))
                _valuation.push(l, true);
            else
                return _valuation;
        }
    }
}

bool DPLL::hasConflict() const
{
    for(const Clause &c : _formula)
        if(_valuation.isClauseFalse(c))
            return true;
    
    return false;
}

Literal DPLL::hasUnitClause() const
{
    Literal l;
    for(const Clause &c : _formula)
        if((l = _valuation.isClauseUnit(c)))
            return l;
        
    return NullLiteral;
}

void DPLL::initWatchlists()
{
    for(const Clause &c : _formula)
        if(c.size() >= 2)
        {
            if(_clauses_watched_literals[c].empty())
            {
                _clauses_watched_literals[c].push_back(c[0]);
                _literals_watchlist[c[0]].push_back(c);
                _clauses_watched_literals[c].push_back(c[1]);
                _literals_watchlist[c[1]].push_back(c);
            }
        }
        else
        {
//             _valuation.push(c[0]);
            _added_unit.push_back(c[0]);
        }
}

OptionalPartialValuation DPLL::solve2wl()
{
    bool conflict = false;
    std::vector<Literal> unitPropQueue;
    
    DB(std::cout << "UNIT CLAUSES FROM THE START: ";
    for(auto l : _added_unit)
        std::cout << l << " ";
    std::cout << std::endl;);
    
    for(auto l : _added_unit)
    {
        DB(std::cout << "UNIT PROPING " << l << std::endl);
        _valuation.push(l);
        DB(std::cout << "UPDATING WATCHLISTS FOR " << -l << std::endl);
        updateWatchlists(-l, &unitPropQueue);
    }
    
    while(unitPropQueue.size() > 0)
    {
        DB(std::cout << "UNIT PROP QUEUE IS ";
        for(auto a : unitPropQueue)
            std::cout << a << " ";
        std::cout << std::endl;);
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
        DB(std::cout << _valuation << std::endl;);
        if(unitPropQueue.size() > 0)
        {
            DB(std::cout << "UNIT PROP QUEUE IS ";
            for(auto a : unitPropQueue)
                std::cout << a << " ";
            std::cout << std::endl;);
            bool conf = false;
            while(!conf && unitPropQueue.size() > 0)
            {
                Literal l = unitPropQueue.back();
                unitPropQueue.pop_back();
                if(_valuation.isLiteralUndefined(l))
                {
                    DB(std::cout << "UNIT PROPING " << l << std::endl);
                    _valuation.push(l);
                    DB(std::cout << "UPDATING WATCHLISTS FOR " << -l << std::endl);
                    conf = updateWatchlists(-l, &unitPropQueue);
                    if(conf)
                        break;
                }
                else
                {
                    if((l > 0 && _valuation.isLiteralTrue(l)) || (l < 0 && _valuation.isLiteralFalse(l)))
                    {
                    DB(std::cout << "CANNOT UNIT PROP " << l << " REPORTING CONFLICT" << std::endl);
                    conf = true;
                    break;
                    }
                }
            }
            conflict = conf;
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
            if(conflict)
                unitPropQueue.clear();
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
                if(conflict)
                    unitPropQueue.clear();
            }
            else
            {
//                 std::cout << "============================================" << std::endl;
//                 for(const Clause &c : _formula)
//                     if(_valuation.isClauseFalse(c))
//                     {
//                         for(auto l : c)
//                             std::cout << l << " ";
//                         std::cout << std::endl;
//                     }
//                 std::cout << "============================================" << std::endl << (hasConflict() ? "UNSAT" : "SAT") << std::endl;
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
        for(auto li : c)
            std::cout << li << " ");
        for(const Literal lit : c)
        {
            DB(std::cout << lit << "(" << (_valuation.isLiteralTrue(lit) ? 1 : (_valuation.isLiteralUndefined(lit) ? 0 : -1)) << ") ";);
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
            for(auto li : c)
                std::cout << li << " ");
        if(foundTrue)
        {
            DB(std::cout << "FOUND TRUE" << std::endl);
            continue;
        }
        else if(foundUndefined)
        {
            DB(std::cout << "FOUND UNDEFINED, SWITCHING WATCHED LITERAL TO " << undef << std::endl);
            if(_clauses_watched_literals[c][0] == l)
            {
                _clauses_watched_literals[c][0] = undef;
                _literals_watchlist[undef].push_back(c);
            }
            else
            {
                _clauses_watched_literals[c][1] = undef;
                _literals_watchlist[undef].push_back(c);
            }
            unwatch.push_back(c);
        }
        else
        {
            if(_clauses_watched_literals[c][0] == l)
            {
                if(_valuation.isLiteralUndefined(_clauses_watched_literals[c][1]))
                {
                    auto tmp = std::find(unitPropQueue->begin(), unitPropQueue->end(), -_clauses_watched_literals[c][1]);
                    if(tmp == unitPropQueue->end())
                    {
                        DB(std::cout << "ADDING " << _clauses_watched_literals[c][1] << " TO UNIT PROP QUEUE" << std::endl);
                        unitPropQueue->push_back(_clauses_watched_literals[c][1]);
                    }
                    else
                    {
                        DB(std::cout << "VARIABLE " << std::abs(_clauses_watched_literals[c][1]) << " ALREADY IN VALUATION, REPORTING CONFLICT" << std::endl);
                        return true;
                    }
                }
                else
                {
                    DB(std::cout << "ALL FALSE, REPORTING CONFLICT" << std::endl);
                    for(auto c : unwatch)
                        _literals_watchlist[l].erase(std::remove(_literals_watchlist[l].begin(), _literals_watchlist[l].end(), c), _literals_watchlist[l].end());
                    return true;
                }
            }
            else
            {
                if(_valuation.isLiteralUndefined(_clauses_watched_literals[c][0]))
                {
                    auto tmp = std::find(unitPropQueue->begin(), unitPropQueue->end(), -_clauses_watched_literals[c][0]);
                    if(tmp == unitPropQueue->end())
                    {
                        DB(std::cout << "ADDING " << _clauses_watched_literals[c][0] << " TO UNIT PROP QUEUE" << std::endl);
                        unitPropQueue->push_back(_clauses_watched_literals[c][0]);
                    }
                    else
                    {
                        DB(std::cout << "VARIABLE " << std::abs(_clauses_watched_literals[c][0]) << " ALREADY IN VALUATION, REPORTING CONFLICT" << std::endl);
                        return true;
                    }
                }
                else
                {
                    DB(std::cout << "ALL FALSE, REPORTING CONFLICT" << std::endl);
                    for(auto c : unwatch)
                        _literals_watchlist[l].erase(std::remove(_literals_watchlist[l].begin(), _literals_watchlist[l].end(), c), _literals_watchlist[l].end());
                    return true;
                }
            }
        }
    }
    
    for(auto c : unwatch)
        _literals_watchlist[l].erase(std::remove(_literals_watchlist[l].begin(), _literals_watchlist[l].end(), c), _literals_watchlist[l].end());
    
    return false;
}
