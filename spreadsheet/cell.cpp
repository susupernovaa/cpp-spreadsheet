#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl {
public:
    std::string cell_text;
    Value cell_value;

    virtual void InvalidateCache() {};

    virtual void Set(std::string text, const SheetInterface& sheet) = 0;

    void Clear() {
        cell_text.clear();
        cell_value = 0.0;
    }

    virtual Value GetValue(const SheetInterface& sheet) const {
        return cell_value;
    }

    std::string GetText() const {
        return cell_text;
    }

    virtual std::vector<Position> GetCellRefs() {
        return {};
    }
};

class Cell::EmptyImpl : public Impl {
public:
    void Set(std::string text, const SheetInterface& sheet) override {
        Clear();
    }
};

class Cell::TextImpl : public Impl {
public:
    void Set(std::string text, const SheetInterface& sheet) override {
        cell_text = std::move(text);

        if (cell_text[0] == '\'') {
            cell_value = cell_text.substr(1);
            return;
        }

        cell_value = cell_text;
    }
};

class Cell::FormulaImpl : public Impl {
public:
    void InvalidateCache() override {
        cache_.reset();
    }

    void Set(std::string text, const SheetInterface& sheet) override {
        formula_ = ParseFormula(text.substr(1));
        cell_text = '=' + formula_->GetExpression();
        InvalidateCache();
    }

    Value GetValue(const SheetInterface& sheet) const override {
        if (!formula_) {
            return FormulaError(FormulaError::Category::Value);
        }
        if (formula_ && !cache_) {
            CalculateCache(sheet);
        }
        return *cache_;
    }

    std::vector<Position> GetCellRefs() override {
        if (!formula_) {
            return {};
        }
        return formula_->GetReferencedCells();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<CellInterface::Value> cache_;

    void CalculateCache(const SheetInterface& sheet) const {
        auto formula_value = formula_->Evaluate(sheet);
        if (std::holds_alternative<double>(formula_value)) {
            cache_ = std::get<double>(formula_value);
        } else if (std::holds_alternative<FormulaError>(formula_value)) {
            cache_ = std::get<FormulaError>(formula_value);
        }
    }
};

Cell::Cell(Sheet& sheet) 
    : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {
}

Cell::~Cell() = default;

void Cell::InvalidateCache() {
    impl_->InvalidateCache();
    for (Cell* cell : refs_to_cell_) {
        cell->InvalidateCache();
    }
}

void Cell::Set(std::string text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text[0] == '=' && text.length() > 1) {
        impl_ = std::make_unique<FormulaImpl>();
    } else {
        impl_ = std::make_unique<TextImpl>();
    }
    impl_->Set(std::move(text), sheet_);
    UpdateCellRefs();
}

void Cell::Clear() {
    impl_->Clear();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue(sheet_);
}
std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return cell_refs_;
}

bool Cell::IsReferenced() const {
    return refs_to_cell_.empty();
}

void Cell::UpdateCellRefs() {
    for (auto old_ref : cell_refs_) {
        Cell* old_referenced_cell = sheet_.GetConcreteCell(old_ref);
        if (old_referenced_cell) {
            old_referenced_cell->refs_to_cell_.erase(this);
        }
    }

    cell_refs_.clear();

    for (const auto& pos : impl_->GetCellRefs()) {
        Cell* referenced_cell = sheet_.GetConcreteCell(pos);
        if (!referenced_cell) {
            sheet_.SetCell(pos, "");
            referenced_cell = sheet_.GetConcreteCell(pos);
        }
        referenced_cell->refs_to_cell_.insert(this);
        cell_refs_.push_back(pos);
    }
}