//
// Created by cgilab on 10/18/18.
//
#pragma once

#ifndef THREES_PUZZLE_AI_NTUPLENETWORK_H
#define THREES_PUZZLE_AI_NTUPLENETWORK_H

#include <vector>
#include <iostream>
#include <fstream>
#include <memory>
#include <array>
#include "Board64.h"


class Tuple {
public:
    Tuple() = default;

    virtual float GetValue(Board64 board, int hint) { return 0; }

    virtual board_t GetIndex(Board64 board, int hint, int id) { return 0; }

    virtual void UpdateValue(Board64 b, int hint, float delta) {}

    virtual void save(std::ofstream &out) {}

    virtual void load(std::ifstream &in) {}
};

class AxeTuple : public Tuple {
public:
    AxeTuple() : Tuple() {
        std::fill(lookup_table_[0].begin(), lookup_table_[0].end(), 0);
        std::fill(lookup_table_[1].begin(), lookup_table_[1].end(), 0);
    }

    board_t GetIndex(Board64 board, int hint, int id) override {
        board_t c1 = board.GetCol(id);
        board_t c2 = board.GetCol(id + 1);
        board_t index = ((((c1 & 0xffff) << 8) | ((c2 & 0xff00) >> 8)) << 2) | (std::min(4, hint) - 1);

        return index;
    }

    void UpdateValue(Board64 board, int hint, float delta) override {
        Board64 b = board;

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; j++) {
                Board64 temp_board = b;

                board_t index = GetIndex(temp_board, hint, j);
                lookup_table_[j][index] += delta;

                temp_board.ReflectVertical();

                index = GetIndex(temp_board, hint, j);
                lookup_table_[j][index] += delta;
            }
            b.TurnRight();
        }
    }

    float GetValue(Board64 board, int hint) override {
        float total_value = 0.0;

        Board64 b = board;

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; ++j) {
                Board64 temp_board = b;

                board_t index = GetIndex(temp_board, hint, j);
                total_value += lookup_table_[j][index];

                temp_board.ReflectVertical();

                index = GetIndex(temp_board, hint, j);
                total_value += lookup_table_[j][index];
            }
            b.TurnRight();
        }

        return total_value;
    }

    void save(std::ofstream &out) override {
        out.write(reinterpret_cast<char *>(&lookup_table_[0][0]), (SIX_TUPLE_AND_HINT_SIZE) * sizeof(float));
        out.write(reinterpret_cast<char *>(&lookup_table_[1][0]), (SIX_TUPLE_AND_HINT_SIZE) * sizeof(float));
    }

    void load(std::ifstream &in) override {
        in.read(reinterpret_cast<char *>(&lookup_table_[0][0]), (SIX_TUPLE_AND_HINT_SIZE) * sizeof(float));
        in.read(reinterpret_cast<char *>(&lookup_table_[1][0]), (SIX_TUPLE_AND_HINT_SIZE) * sizeof(float));
    }

private:
    std::array<std::array<float, SIX_TUPLE_AND_HINT_SIZE>, 2> lookup_table_;
};

class RectangleTuple : public Tuple {
public:
    RectangleTuple() {
        std::fill(lookup_table_[0].begin(), lookup_table_[0].end(), 0);
        std::fill(lookup_table_[1].begin(), lookup_table_[1].end(), 0);
    }

    board_t GetIndex(Board64 board, int hint, int id) override {
        board_t c1 = board.GetCol(id);
        board_t c2 = board.GetCol(id + 1);
        board_t index = ((c1 & 0xfff) << 12ULL) | (c2 & 0xfff);

        return (std::min(4, hint) - 1) | (index << 2);
    }

    void UpdateValue(Board64 board, int hint, float delta) override {
        Board64 b = board;

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; j++) {
                Board64 temp_board = b;

                board_t index1 = GetIndex(temp_board, hint, j);
                lookup_table_[j][index1] += delta;

                temp_board.ReflectVertical();

                board_t index2 = GetIndex(temp_board, hint, j);
                if (j == 1 && index1 != index2) {
                    lookup_table_[j][index2] += delta;
                }
            }
            b.TurnRight();
        }
    }

    float GetValue(Board64 board, int hint) override {
        float total_value = 0.0;

        Board64 b = board;

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; ++j) {
                Board64 temp_board = b;

                board_t index1 = GetIndex(temp_board, hint, j);
                total_value += lookup_table_[j][index1];

                temp_board.ReflectVertical();

                board_t index2 = GetIndex(temp_board, hint, j);
                if (j == 1 && index1 != index2) {
                    total_value += lookup_table_[j][index2];
                }
            }
            b.TurnRight();
        }

        return total_value;
    }

    void save(std::ofstream &out) override {
        out.write(reinterpret_cast<char *>(&lookup_table_[0][0]), (SIX_TUPLE_AND_HINT_SIZE) * sizeof(float));
        out.write(reinterpret_cast<char *>(&lookup_table_[1][0]), (SIX_TUPLE_AND_HINT_SIZE) * sizeof(float));
    }

    void load(std::ifstream &in) override {
        in.read(reinterpret_cast<char *>(&lookup_table_[0][0]), (SIX_TUPLE_AND_HINT_SIZE) * sizeof(float));
        in.read(reinterpret_cast<char *>(&lookup_table_[1][0]), (SIX_TUPLE_AND_HINT_SIZE) * sizeof(float));
    }

private:
    std::array<std::array<float, SIX_TUPLE_AND_HINT_SIZE>, 2> lookup_table_;
};

class ValuableTileTuple : public Tuple {
public:
    ValuableTileTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(Board64 board, int hint, int id) override {
        Board64 b = board;

        int count_tile[15];
        std::fill(count_tile, count_tile + 15, 0);

        for (int i = 0; i < 16; ++i) {
            count_tile[b(i)]++;
        }

        board_t index = 0;
        for (int i = 0; i < 5; i++) {
            index = (index << 4) | (count_tile[i + 10]);
        }

        return (std::min(4, hint) - 1) | (index << 2);
    }

    void UpdateValue(Board64 board, int hint, float delta) override {
        lookup_table_[GetIndex(board, hint, 0)] += delta;
    }

    float GetValue(Board64 board, int hint) override {
        return lookup_table_[GetIndex(board, hint, 0)];
    }

    void save(std::ofstream &out) override {
        out.write(reinterpret_cast<char *>(&lookup_table_[0]), 4194304 * sizeof(float));
    }

    void load(std::ifstream &in) override {
        in.read(reinterpret_cast<char *>(&lookup_table_[0]), 4194304 * sizeof(float));
    }

private:
    std::array<float, 4194304> lookup_table_; // (hint-tile, 10-tile, 11-tile, 12-tile, 13-tile, 14-tile)
};

class EmptyTileTuple : public Tuple {
public:
    EmptyTileTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(Board64 board, int hint, int id) override {
        board_t index = 0;

        for (int i = 0; i < 16; ++i) {
            index += (board(i) == 0);
        }

        return (index << 2) | (std::min(4, hint) - 1);
    }

    void UpdateValue(Board64 board, int hint, float delta) override {
        lookup_table_[GetIndex(board, hint, 0)] += delta;
    }

    float GetValue(Board64 board, int hint) override {
        return lookup_table_[GetIndex(board, hint, 0)];
    }

    void save(std::ofstream &out) override {
        out.write(reinterpret_cast<char *>(&lookup_table_[0]), 68 * sizeof(float));
    }

    void load(std::ifstream &in) override {
        in.read(reinterpret_cast<char *>(&lookup_table_[0]), 68 * sizeof(float));
    }

private:
    std::array<float, 68> lookup_table_;
};

class DistinctTilesTuple : public Tuple {
public:
    DistinctTilesTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(Board64 board, int hint, int id) override {
        board_t index = 0;

        for (int i = 0; i < 16; ++i) {
            index |= (1 << board(i));
        }

        return (std::min(4, hint) - 1) | (index << 2);
    }

    void UpdateValue(Board64 board, int hint, float delta) override {
        lookup_table_[GetIndex(board, hint, 0)] += delta;
    }

    float GetValue(Board64 board, int hint) override {
        return lookup_table_[GetIndex(board, hint, 0)];
    }

    void save(std::ofstream &out) override {
        out.write(reinterpret_cast<char *>(&lookup_table_[0]), 262144 * sizeof(float));
    }

    void load(std::ifstream &in) override {
        in.read(reinterpret_cast<char *>(&lookup_table_[0]), 262144 * sizeof(float));
    }

private:
    std::array<float, 262144> lookup_table_;
};

class MergeableTilesTuple : public Tuple {
public:
    MergeableTilesTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(Board64 board, int hint, int id) override {
        board_t index = 0;

        for (int i = 0; i < 16; ++i) {
            if (board(i) == 0) continue;

            if ((i + 1) % 4 != 0 && board(i) == board(i + 1)) {
                index++;
            }
            if ((i + 4) / 4 < 4 && board(i) == board(i + 4)) {
                index++;
            }
        }

        return (index << 2) | (std::min(4, hint) - 1);
    }

    void UpdateValue(Board64 board, int hint, float delta) override {
        lookup_table_[GetIndex(board, hint, 0)] += delta;
    }

    float GetValue(Board64 board, int hint) override {
        return lookup_table_[GetIndex(board, hint, 0)];
    }

    void save(std::ofstream &out) override {
        out.write(reinterpret_cast<char *>(&lookup_table_[0]), 68 * sizeof(float));
    }

    void load(std::ifstream &in) override {
        in.read(reinterpret_cast<char *>(&lookup_table_[0]), 68 * sizeof(float));
    }

private:
    std::array<float, 68> lookup_table_;
};

class NeighboringVTile : public Tuple {
public:
    NeighboringVTile() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(Board64 board, int hint, int id) override {
        board_t index = 0;

        for (int i = 0; i < 16; ++i) {
            if (board(i) < 10) continue;

            if ((i + 1) % 4 != 0 && (board(i) - 1 == board(i + 1) || board(i) + 1 == board(i + 1))) {
                index++;
            }
            if ((i + 4) / 4 < 4 && (board(i) - 1 == board(i + 1) || board(i) + 1 == board(i + 1))) {
                index++;
            }
        }

        return (index << 2) | (std::min(4, hint) - 1);
    }

    void UpdateValue(Board64 board, int hint, float delta) override {
        lookup_table_[GetIndex(board, hint, 0)] += delta;
    }

    float GetValue(Board64 board, int hint) override {
        return lookup_table_[GetIndex(board, hint, 0)];
    }

    void save(std::ofstream &out) override {
        out.write(reinterpret_cast<char *>(&lookup_table_[0]), 68 * sizeof(float));
    }

    void load(std::ifstream &in) override {
        in.read(reinterpret_cast<char *>(&lookup_table_[0]), 68 * sizeof(float));
    }

private:
    std::array<float, 68> lookup_table_;
};

class NTupleNetwork {

public:
    NTupleNetwork() {
        tuples.emplace_back(new AxeTuple());
        tuples.emplace_back(new RectangleTuple());
        tuples.emplace_back(new ValuableTileTuple());
        tuples.emplace_back(new DistinctTilesTuple());
        tuples.emplace_back(new MergeableTilesTuple());
        tuples.emplace_back(new EmptyTileTuple());
        tuples.emplace_back(new NeighboringVTile());
    }

    float GetValue(Board64 board, int hint) {
        float total_value = 0;
        for (auto &tuple : tuples) {
            total_value += tuple->GetValue(board, hint);
        }


        return total_value;
    }

    void UpdateValue(Board64 board, int hint, float delta) {
        for (auto &tuple : tuples) {
            tuple->UpdateValue(board, hint, delta);
        }
    }

    void save(std::ofstream &save_stream) {
        for (auto &tuple : tuples) {
            tuple->save(save_stream);
        }
    }

    void load(std::ifstream &load_stream) {
        for (auto &tuple : tuples) {
            tuple->load(load_stream);
        }
    }

private:
    std::vector<std::unique_ptr<Tuple>> tuples;
};


#endif //THREES_PUZZLE_AI_NTUPLENETWORK_H
