#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category) 
    : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError other) const {
    return category_ == other.category_;
}

std::ostream& operator<<(std::ostream& output, const FormulaError& error) {
    switch (error.GetCategory()) {
        case FormulaError::Category::Ref:
            return output << "#REF!";
        case FormulaError::Category::Value:
            return output << "#VALUE!";
        default:
            output << "#ARITHM!";
    }
    return output;
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try 
        : ast_(ParseFormulaAST(expression)) {
    } catch (const std::exception& e) {
        throw FormulaException(e.what());
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        Value result;
        try {
            result = ast_.Execute(sheet);
        } catch (const FormulaError& error) {
            result = FormulaError(error.GetCategory());
        }
        return result;
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> result;
        auto cells = ast_.GetCells();
        std::copy(cells.begin(), cells.end(), std::back_inserter(result));
        auto last = std::unique(result.begin(), result.end());
        result.erase(last, result.end());
        return result;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}