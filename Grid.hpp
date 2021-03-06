#pragma once

#include <string>
#include <chrono>
#include <string_view>
#include <memory>
#include <vector>
#include <utility>
#include <random>
#include <set>
#include <map>
#include <string>
#include <regex>
#include <thread>
#include <mutex>


class Grid {
public:
    using steady_clock_tp = std::chrono::high_resolution_clock::time_point;
    using set_pair_t = std::set<std::pair<int, int>>;

    Grid(int width, int height, std::string_view s);

    enum struct SitRep {
        CONTRADICTION_FOUND,
        SOLUTION_FOUND,
        KEEP_GOING,
        CANNOT_PROCEED,
    };

    SitRep solve(bool verbose = true, bool guessing = true);
    int knownElements() const;
    void write(std::ostream& os, steady_clock_tp start, steady_clock_tp finish) const;

private:
    enum struct State : int {
        UNKNOWN = -3,
        WHITE = -2,
        BLACK = -1,
    };
#pragma region
    class Region {
    public:
        Region(State const state, set_pair_t const& unknowns, int const x, int const y);
        constexpr bool is_white() const noexcept { return m_state == State::WHITE; }
        constexpr bool is_black() const noexcept { return m_state == State::BLACK; }
        constexpr bool is_numbered() const noexcept { return static_cast<int>(m_state) > 0; }
        int its_number() const noexcept;
        set_pair_t::const_iterator begin() const;
        set_pair_t::const_iterator end() const;
        int size() const noexcept;
        bool contains(int const x, int const y) const noexcept;

        template <typename It>
        inline void insert(It first, It last);

        set_pair_t::const_iterator unk_begin() const;
        set_pair_t::const_iterator unk_end() const;

        template <typename It>
        inline void unk_insert(It first, It last);

        int unk_size() const noexcept;
        void unk_erase(int const x, int const y) noexcept;

    private:
        State m_state;

        //Tracks the regions.
        set_pair_t m_coords;

        //Track what unknowns cells surround the region.
        set_pair_t m_unknowns;
    };
#pragma endregion

    using cache_map_t = std::map<std::shared_ptr<Region>, set_pair_t>;

    int m_width;
    int m_height;

    std::mutex mt;

    //The total black cells in the solution.
    int m_total_black;

    //m_cells[x][y].first is the state of the cell.
    //m_cells[x][y].second is the region of the cell.
    std::vector<std::vector<std::pair<State, std::shared_ptr<Region>>>> m_cells;

    //Initially is KEEP_GOING.
    SitRep m_sitRep;

    std::set<std::shared_ptr<Region>> m_regions;

    //This stores the output to be generated and converts into HTML.
    std::vector<std::tuple<std::string_view, std::vector<std::vector<State>>,
        set_pair_t, steady_clock_tp, int, set_pair_t>> m_output;

    static std::regex const rx;
    std::mt19937 m_eng;
    //std::string m_string;


    Grid(Grid const& other);

    [[nodiscard]] bool analyze_complete_islands(bool  verbose);
    [[nodiscard]] bool analyze_single_liberty(bool verbose);
    [[nodiscard]] bool analyze_dual_liberties(bool verbose);
    [[nodiscard]] bool analyze_unreachable_cells(bool verbose);
    [[nodiscard]] bool analyze_potential_pools(bool verbose);
    [[nodiscard]] bool analyze_confinement(bool verbose, cache_map_t& cache);
    [[nodiscard]] bool analyze_hypotheticals(bool verbose);

    std::vector<std::pair<int, int>> guessing_order();
    [[nodiscard]] bool valid(int x, int y);

    State& cell(int x, int y);
    [[nodiscard]] State const& cell(int x, int y) const;

    std::shared_ptr<Region>& region(int x, int y);
    [[nodiscard]] std::shared_ptr<Region> const& region(int x, int y) const;

    void print(std::string_view s, set_pair_t const& updated = {},
               int failed_guesses = 0, set_pair_t const& failed_coords = {});

    [[nodiscard]] bool process(bool verbose, set_pair_t const& mark_as_black,
                               set_pair_t const& mark_as_white, std::string_view s, int const failed_guesses = 0,
                               set_pair_t const& failed_coords = {});

    template <typename F>
    void for_valid_neighbors(int x, int y, F f) const;

    void insert_valid_neighbors(set_pair_t& s, int x, int y) const;
    void insert_valid_unknown_neighbors(set_pair_t& s, int x, int y) const;

    void add_region(int x, int y);
    void mark(State const state, int x, int y);
    void fuse_regions(std::shared_ptr<Region> r1, std::shared_ptr<Region> r2);

    [[nodiscard]] bool impossibly_big_white_region(int n) const;
    [[nodiscard]] bool unreachable(int x_root, int y_root, set_pair_t discovered = {});
    [[nodiscard]] bool confined(std::shared_ptr<Region> const& r, cache_map_t& cache, set_pair_t const& verboten = {});

    bool detect_contradictions(bool verbose, cache_map_t& cache);

};

//Helper function for formatting time and prints it to std::ostream.
std::string format_time(Grid::steady_clock_tp const start, Grid::steady_clock_tp const finish);

//member function templates.
template <typename It>
inline void Grid::Region::insert(It first, It last) {
    m_coords.insert(first, last);
}
template <typename It>
inline void Grid::Region::unk_insert(It first, It last) {
    m_unknowns.insert(first, last);
}
template <typename  F>
void Grid::for_valid_neighbors(int x, int y, F f) const { 
    if (x > 0) {
        f(x - 1, y);
    }
    if (x + 1 < m_width) {
        f(x + 1, y);
    }
    if (y > 0) {
        f(x, y - 1);
    }
    if (y + 1 < m_height) {
        f(x, y + 1);
    }
}


