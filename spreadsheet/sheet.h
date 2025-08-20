#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Cell;

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);

private:
    void MaybeIncreaseSizeToIncludePosition(Position pos);
    void PrintCells(std::ostream& output,
                    const std::function<void(std::ostream&, const CellInterface&)>& printCell) const;
    Size GetActualSize() const;

    void UpdatePrintableSize();

    bool IsAccessible(Position pos) const;

    void CheckForCycles(Position pos);
    bool DFS(CellInterface* cell, std::unordered_set<CellInterface*>& visited, std::unordered_set<CellInterface*>& recursion_stack);

    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
    Size printable_size_;
};