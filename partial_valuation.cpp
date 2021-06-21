#include "partial_valuation.h"

#include <algorithm>
#include <cstdint>

PartialValuation::PartialValuation(unsigned nVars)
: _values(nVars + 1, ExtendedBool::Undefined),
  _stack()
{
    _stack.reserve(nVars);
}

void PartialValuation::push(Literal l, bool decide)
{
    _values[std::abs(l)] = l > 0 ? ExtendedBool::True : ExtendedBool::False;
    if(decide)
        _stack.push_back(NullLiteral);
    _stack.push_back(l);
}

Literal PartialValuation::backtrack()
{
    if(_stack.empty())
        return NullLiteral;
    
    Literal lastDecided = NullLiteral, last = NullLiteral;
    do
    {
        last = _stack.back();
        _stack.pop_back();
        
        if(last == NullLiteral)
            break;
        
        _values[std::abs(last)] = ExtendedBool::Undefined;
        lastDecided = last;
    } while(_stack.size());
    
    return last == NullLiteral ? lastDecided : NullLiteral;
}

Literal PartialValuation::firstUndefined() const
{
    auto it = std::find(_values.cbegin() + 1, _values.cend(), ExtendedBool::Undefined);
    return it != _values.cend() ? it - _values.cbegin() : NullLiteral;
}

void PartialValuation::reset(unsigned nVars)
{
    _values.resize(nVars + 1);
    std::fill(_values.begin(), _values.end(), ExtendedBool::Undefined);
    
    _stack.clear();
    _stack.reserve(nVars);
}

std::ostream &operator<<(std::ostream &out, const PartialValuation &pv)
{
    out << "[ ";
    for(std::size_t i = 1; i < pv._values.size(); i++)
    {
        if(pv._values[i] == ExtendedBool::True)
            out << 'p' << i << " ";
        else if(pv._values[i] == ExtendedBool::False)
            out << "~p" << i << " ";
        else if(pv._values[i] == ExtendedBool::Undefined)
            out << 'u' << i << " ";
        else
            throw std::logic_error("Unexpected value assigned to a variable");
    }

    out << "]\t||\t STACK: ";

    for(const auto l : pv._stack)
    	if(l != 0)
        	out << l << " ";
        else
        	out << "| ";

    return out;
}

void PartialValuation::printFullValuation(std::ostream &out) const
{
	for(std::size_t i = 1; i < _values.size(); i++)
    {
        if(_values[i] == ExtendedBool::True)
            out << i << " ";
        else if(_values[i] == ExtendedBool::False)
            out << "-" << i << " ";
        else
            throw std::logic_error("Undefined value in a full valuation");
    }
}

bool PartialValuation::isLiteralUndefined(Literal l) const
{
    return _values[std::abs(l)] == ExtendedBool::Undefined ? true : false;
}

bool PartialValuation::isLiteralFalse(Literal l) const
{
    if(l > 0)
        return _values[std::abs(l)] == ExtendedBool::False ? true : false;
    else
        return _values[std::abs(l)] == ExtendedBool::True ? true : false;
}

bool PartialValuation::isLiteralTrue(Literal l) const
{
    if(l > 0)
        return _values[std::abs(l)] == ExtendedBool::True ? true : false;
    else
        return _values[std::abs(l)] == ExtendedBool::False ? true : false;
}
