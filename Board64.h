#pragma once

#include <array>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <cstdlib>

#include "Common.h"
#include "LookUpTable.h"

/**
 * 64bit board for Threes
 * A Row or a Column is 16bit
 * A tile is 4bit
 *
 * index (1-d form):
 *  (0)  (1)  (2)  (3)
 *  (4)  (5)  (6)  (7)
 *  (8)  (9) (10) (11)
 * (12) (13) (14) (15)
 *
 */

static float ScoreHelper(board_t board, const float *table) {
    return table[(board >> 0) & ROW_MASK] +
           table[(board >> 16) & ROW_MASK] +
           table[(board >> 32) & ROW_MASK] +
           table[(board >> 48) & ROW_MASK];
}

static float GetBoardScore(board_t board) {
    return ScoreHelper(board, score_table);
}

static board_t Transpose(board_t board) {
    board_t a1 = board & 0xF0F00F0FF0F00F0FULL;
    board_t a2 = board & 0x0000F0F00000F0F0ULL;
    board_t a3 = board & 0x0F0F00000F0F0000ULL;
    board_t a = a1 | (a2 << 12) | (a3 >> 12);
    board_t b1 = a & 0xFF00FF0000FF00FFULL;
    board_t b2 = a & 0x00FF00FF00000000ULL;
    board_t b3 = a & 0x00000000FF00FF00ULL;
    return b1 | (b2 >> 24) | (b3 << 24);
}

class Board64 {
public:
    Board64() : board_(), hint_(0) {}

    Board64(const board_t &board, const int hint = 0) : board_(board), hint_(hint) {}

    Board64(const Board64 &board) = default;

    Board64 &operator=(const Board64 &b) = default;

    explicit operator board_t &() { return board_; }

    explicit operator const board_t &() const { return board_; }

    row_t operator[](unsigned i) {
        return row_t((board_ >> (i * 16)) & ROW_MASK);
    }

    const row_t operator[](unsigned i) const {
        return row_t((board_ >> (i * 16)) & ROW_MASK);
    }

    board_t GetBoard() {
        return board_;
    }

    board_t GetBoard() const {
        return board_;
    }

    int operator()(int i) {
        int row_id = i / 4;
        int col_id = i % 4;

        row_t row = row_t((this->board_ >> (row_id * 16)) & ROW_MASK);
        cell_t cell = ((row >> (col_id * 4)) & 0xf);

        return cell;
    }

    const int operator()(int i) const {
        int row_id = i / 4;
        int col_id = i % 4;

        row_t row = row_t((this->board_ >> (row_id * 16)) & ROW_MASK);
        cell_t cell = ((row >> (col_id * 4)) & 0xf);

        return cell;
    }

    bool operator==(const Board64 &b) const { return board_ == b.board_; }

    bool operator<(const Board64 &b) const { return board_ < b.board_; }

    bool operator!=(const Board64 &b) const { return !(*this == b); }

    bool operator>(const Board64 &b) const { return b < *this; }

    bool operator<=(const Board64 &b) const { return !(b < *this); }

    bool operator>=(const Board64 &b) const { return !(*this < b); }

    friend std::ostream &operator<<(std::ostream &out, const Board64 &board) {
        out << "+------------------------+" << std::endl;
        for (int i = 0; i < 4; i++) {
            out << "|" << std::dec;
            for (int j = 0; j < 4; j++) {
                uint8_t tile = board(i * 4 + j);
                int value = tile >= 3 ? ((1 << (tile - 3)) * 3) : tile;

                out << std::setw(6) << value;
            }
            out << "|" << std::endl;
        }
        out << "+------------------------+" << std::endl;
        return out;
    }

    reward_t Place(unsigned int position, cell_t tile) {
        if (position >= 16) return -1;
//        if (tile != 1 && tile != 2 && tile != 3) return -1;

        board_t row_id = position / 4;
        board_t col_id = position % 4;

        board_t row = board_t(tile) << (row_id * 16);
        board_t cell = row << (col_id * 4);
        board_ = board_ | cell;

        return GetBoardScore(board_);
    }

    /**
     * apply an action to the board
     * return the reward of the action, or -1 if the action is illegal
     */
    reward_t Slide(unsigned int direction) {
        switch (direction & 0b11) {
            case 0:
                return SlideUp();
            case 1:
                return SlideRight();
            case 2:
                return SlideDown();
            case 3:
                return SlideLeft();
            default:
                return -1;
        }
    }

    reward_t SlideLeft() {
        board_t ret = board_;

        ret ^= board_t(row_left_table[(board_ >> 0) & ROW_MASK]) << 0;
        ret ^= board_t(row_left_table[(board_ >> 16) & ROW_MASK]) << 16;
        ret ^= board_t(row_left_table[(board_ >> 32) & ROW_MASK]) << 32;
        ret ^= board_t(row_left_table[(board_ >> 48) & ROW_MASK]) << 48;

        this->board_ = ret;

        return GetBoardScore(ret);
    }

    reward_t SlideRight() {
        board_t ret = board_;

        ret ^= board_t(row_right_table[(board_ >> 0) & ROW_MASK]) << 0;
        ret ^= board_t(row_right_table[(board_ >> 16) & ROW_MASK]) << 16;
        ret ^= board_t(row_right_table[(board_ >> 32) & ROW_MASK]) << 32;
        ret ^= board_t(row_right_table[(board_ >> 48) & ROW_MASK]) << 48;

        this->board_ = ret;

        return GetBoardScore(ret);
    }

    reward_t SlideUp() {
        board_t ret = board_;
        board_t transpose_board = ::Transpose(board_);

        ret ^= col_up_table[(transpose_board >> 0) & ROW_MASK] << 0;
        ret ^= col_up_table[(transpose_board >> 16) & ROW_MASK] << 4;
        ret ^= col_up_table[(transpose_board >> 32) & ROW_MASK] << 8;
        ret ^= col_up_table[(transpose_board >> 48) & ROW_MASK] << 12;
        this->board_ = ret;

        return GetBoardScore(ret);
    }

    reward_t SlideDown() {
        board_t ret = board_;
        board_t transpose_board = ::Transpose(board_);

        ret ^= col_down_table[(transpose_board >> 0) & ROW_MASK] << 0;
        ret ^= col_down_table[(transpose_board >> 16) & ROW_MASK] << 4;
        ret ^= col_down_table[(transpose_board >> 32) & ROW_MASK] << 8;
        ret ^= col_down_table[(transpose_board >> 48) & ROW_MASK] << 12;
        this->board_ = ret;

        return GetBoardScore(ret);
    }

    float GetHeuristicScore() {
        return ScoreHelper(board_, heur_score_table) +
               ScoreHelper(::Transpose(board_), heur_score_table);
    }

    cell_t GetMaxTile() {
        return std::max(row_max_table[(board_ >> 0) & ROW_MASK],
                        std::max(row_max_table[(board_ >> 16) & ROW_MASK],
                                 std::max(row_max_table[(board_ >> 32) & ROW_MASK],
                                          row_max_table[(board_ >> 48) & ROW_MASK])));
    }

    cell_t GetMaxTile() const {
        return std::max(row_max_table[(board_ >> 0) & ROW_MASK],
                        std::max(row_max_table[(board_ >> 16) & ROW_MASK],
                                 std::max(row_max_table[(board_ >> 32) & ROW_MASK],
                                          row_max_table[(board_ >> 48) & ROW_MASK])));
    }

    row_t GetRow(int row){
        return row_t((board_ >> (16ULL*row)) & ROW_MASK);
    }

    row_t GetCol(int col) {
        board_t transpose_board = ::Transpose(board_);
        return row_t((transpose_board >> 16ULL*col) & ROW_MASK);
    }

    void SetRow(int row_id, row_t value) {
        board_ = (board_ ^ (board_t(GetRow(row_id)) << (16ULL * row_id))) | (board_t(value) << (16ULL * row_id));
    }

    static void PrintBoard(board_t board) {
        int i,j;
        for(i=0; i<4; i++) {
            for(j=0; j<4; j++) {
                printf("%c", "0123456789abcdef"[(board)&0xf]);
                board >>= 4;
            }
            printf("\n");
        }
        printf("\n");
    }

    void TurnRight() {
        this->Transpose();
        this->ReflectVertical();
    }

    void ReflectVertical() {
        for (int r = 0; r < 4; r++) {
            this->ReverseRow(r);
        }
    }

    void Transpose() {
        board_ = ::Transpose(this->board_);
    }

    int GetHint() {
        return hint_;
    }

    const int GetHint() const {
        return hint_;
    }

    void SetHint(int hint) {
        hint_ = hint;
    }

private:
    board_t board_;
    cell_t largest_tile_;
    int hint_;

    row_t ReverseRow(int row_id) {
        row_t row = GetRow(row_id);
        row = (row&0xf000) >> 12 | (row&0x0f00) >> 4 | (row&0x00f0)<<4 | (row&0x000f) << 12;

        SetRow(row_id, row);
    }
};