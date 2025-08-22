#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <forward_list>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    void InvalidateCache(std::unordered_set<Cell*>& visited);

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    // возвращает индексы всех ячеек, которые входят в формулу
    std::vector<Position> GetReferencedCells() const override;
    
    bool IsReferenced() const;
private:
    // на кого ссылается ячейка
    std::vector<Position> cell_refs_;
    // кто ссылается на ячейку
    std::unordered_set<Cell*> refs_to_cell_;
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;

    void UpdateCellRefs();
};