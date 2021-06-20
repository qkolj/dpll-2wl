#ifndef DPLL_H
#define DPLL_H

#include "partial_valuation.h"

#include <iostream>
#include <experimental/optional>
#include <map>
#include <vector>

using OptionalPartialValuation = std::experimental::optional<PartialValuation>;

class DPLL
{
public:
    DPLL(const CNFFormula &formula);
    DPLL(std::istream &dimacsStream);
    OptionalPartialValuation solve();
    OptionalPartialValuation solve2wl();
    
private:
    bool hasConflict() const;
    Literal hasUnitClause() const;
    void initWatchlists();
    bool updateWatchlists(Literal l, std::vector<Literal>* unitPropQueue);
    
    CNFFormula _formula;
    PartialValuation _valuation;
    std::vector<Literal> _added_unit;
    std::map<Literal, std::vector<Clause>> _literals_watchlist;
    std::map<Clause, std::vector<Literal>> _clauses_watched_literals;
};

#endif // DPLL_H
