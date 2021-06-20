#ifndef PARTIALVALUATION_H
#define PARTIALVALUATION_H

#include <cstdint>
#include <vector>
#include <iostream>

#define NullLiteral (0)

enum class ExtendedBool: uint8_t
{
    False,
    True,
    Undefined
};

using Literal = int;
using Clause = std::vector<Literal>;
using CNFFormula = std::vector<Clause>;

class PartialValuation
{
public:
    PartialValuation(unsigned nVars = 0);
    
    void push(Literal l, bool decide = false);
    Literal backtrack();
    bool isClauseFalse(const Clause &c) const;
    Literal isClauseUnit(const Clause &c) const;
    Literal firstUndefined() const;
    bool isLiteralUndefined(Literal l) const;
    bool isLiteralFalse(Literal l) const;
    bool isLiteralTrue(Literal l) const;
    void reset(unsigned nVars);
    friend std::ostream &operator<<(std::ostream &out, const PartialValuation &pv);
    
private:
    std::vector<ExtendedBool> _values;
    std::vector<Literal> _stack;
};

#endif // PARTIALVALUATION_H
