#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    MaybeIncreaseSizeToIncludePosition(pos);

    if (cells_[pos.row][pos.col] && cells_[pos.row][pos.col]->GetText() == text) {
        return;
    }

    std::unique_ptr<Cell> old_cell;
    if (cells_[pos.row][pos.col]) {
        old_cell = std::move(cells_[pos.row][pos.col]);
    }

    cells_[pos.row][pos.col] = std::make_unique<Cell>(*this);

    {
        std::unordered_set<Cell*> visited_cells;
        cells_[pos.row][pos.col]->InvalidateCache(visited_cells);
    }

    try {
        cells_[pos.row][pos.col]->Set(std::move(text));

        for (Position ref_pos : cells_[pos.row][pos.col]->GetReferencedCells()) {
            CheckForCycles(ref_pos);
        }

        UpdatePrintableSize();

    } catch (const CircularDependencyException& e) {
        if (old_cell) {
            cells_[pos.row][pos.col] = std::move(old_cell);
        } else {
            cells_[pos.row][pos.col].reset();
        }
        UpdatePrintableSize();
        throw; 
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetConcreteCell(pos);
}
CellInterface* Sheet::GetCell(Position pos) {
    return GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (!IsAccessible(pos) || (IsAccessible(pos) && !cells_[pos.row][pos.col])) {
        return;
    }
    if (cells_[pos.row][pos.col]->IsReferenced()) {
        cells_[pos.row][pos.col]->Clear();
    } else {
        cells_[pos.row][pos.col].reset();
    }
    UpdatePrintableSize();
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintCells(output, [](std::ostream& out, const CellInterface& cell) {
        out << cell.GetValue();
    });
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintCells(output, [](std::ostream& out, const CellInterface& cell) {
        out << cell.GetText();
    });
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    return IsAccessible(pos) ? cells_[pos.row][pos.col].get() : nullptr;
}

Cell* Sheet::GetConcreteCell(Position pos) {
    return const_cast<Cell*>(static_cast<const Sheet*>(this)->GetConcreteCell(pos));
}

void Sheet::MaybeIncreaseSizeToIncludePosition(Position pos) {
    size_t new_rows = static_cast<size_t>(pos.row + 1);
    size_t new_cols = static_cast<size_t>(pos.col + 1);

    if (cells_.size() < new_rows) {
        cells_.resize(new_rows);
    }

    for (auto& row : cells_) {
        if (row.size() < std::max(new_cols, static_cast<size_t>(printable_size_.cols))) {
            row.resize(std::max(new_cols, static_cast<size_t>(printable_size_.cols)));
        }
    }
}

void Sheet::PrintCells(std::ostream& output,
                const std::function<void(std::ostream&, const CellInterface&)>& print_cell) const {
    for (int row = 0; row < printable_size_.rows; ++row) {
        for (int col = 0; col < printable_size_.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (cells_[row][col]) {
                print_cell(output, static_cast<const CellInterface&>(*cells_[row][col]));
            }
        }
        output << '\n';
    }
}

Size Sheet::GetActualSize() const {
    return {static_cast<int>(cells_.size()), static_cast<int>(cells_.back().size())};
}

void Sheet::UpdatePrintableSize() {
    printable_size_ = {0, 0};                                                                                                                          
    for (size_t row = 0; row < cells_.size(); ++row) {
        for (size_t col = 0; col < cells_[row].size(); ++col) {
            if (cells_[row][col] && !cells_[row][col]->GetText().empty()) {
                printable_size_.rows = std::max(printable_size_.rows, static_cast<int>(row) + 1);
                printable_size_.cols = std::max(printable_size_.cols, static_cast<int>(col) + 1);
            }
        }
    }
}

bool Sheet::IsAccessible(Position pos) const {
    return pos.row < static_cast<int>(cells_.size()) && pos.col < static_cast<int>(cells_[pos.row].size());
}

void Sheet::CheckForCycles(Position pos) {
    auto* cell_to_check = GetCell(pos);
    if (!cell_to_check) {
        return;
    }

    std::unordered_set<CellInterface*> visited;
    std::unordered_set<CellInterface*> recursion_stack;

    if (DFS(cell_to_check, visited, recursion_stack)) {
        throw CircularDependencyException("Circular dependency occurred");
    }
}

bool Sheet::DFS(CellInterface* cell, std::unordered_set<CellInterface*>& visited, std::unordered_set<CellInterface*>& recursion_stack) {
    if (!cell) {
        return false;
    }

    if (recursion_stack.find(cell) != recursion_stack.end()) {
        return true;
    }

    if (visited.find(cell) != visited.end()) {
        return false;
    }

    visited.insert(cell);
    recursion_stack.insert(cell);

    for (Position pos : cell->GetReferencedCells()) {
        if (DFS(GetCell(pos), visited, recursion_stack)) {
            return true;
        }
    }

    recursion_stack.erase(cell);
    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}