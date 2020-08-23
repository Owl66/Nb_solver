#ifndef GRID_HPP_INCLUDED
#define GRID_HPP_INCLUDED


#include <chrono>
#include <memory>
#include <vector>
#include <utility>
#include <random>
#include <string_view>
#include <iostream>
#include <set>
#include <map>




namespace nb {

class Grid {

public:
    using steady_clock_tp_t = std::chrono::steady_clock::time_point;
    using set_pair_t = std::set<std::pair<int, int>>;

public:
    Grid(Grid const&);
    Grid& operator=(Grid const&) = delete;


public:
    Grid(int width, int height, std::string_view s);

    enum struct SitRep {
        CONTRADICTION_FOUND,
        SOLUTION_FOUND,
        KEEP_GOING,
        CANNOT_PROCEED,
    };

    SitRep solve(bool verbose = true, bool guessing = true);
    int known() const;
    void write(std::ostream& os, steady_clock_tp_t const  start, steady_clock_tp_t const finish) const;

private:
    enum struct State : int {
        UNKNOWN = -3,
        WHITE = -2,
        BLACK = -1,
    };



    class Region {

    public:
        Region(State const state, set_pair_t const& unknowns, int const x, int const y);
        bool is_white() const;
        bool is_black() const;
        bool is_numbered() const;
        int number() const;
        set_pair_t::const_iterator begin() const;
        set_pair_t::const_iterator end() const;
        int size() const;
        bool contains(int const x, int const y) const;

        template <typename It>
        void insert(It first, It last);

        set_pair_t::const_iterator unk_begin() const;
        set_pair_t::const_iterator unk_end() const;

        template <typename It>
        void unk_insert(It first, It last);

        int unk_size() const;
        void unk_erase(int const x, int const y);


    private:
        State m_state;

        //Tracks the regions.
        set_pair_t m_coords{};

        //Track what unknowns cells surround the region.
        set_pair_t m_unknowns;


    };


    using cache_map_t = std::map<std::shared_ptr<Region>, set_pair_t>;


    int m_width;
    int m_height;

    //The total black cells in the solution.
    int m_total_black;

    //m_cells[x][y].first is the state of the cell.
    //m_cells[x][y].second is the region of the cell.
    std::vector<std::vector<std::pair<State, std::shared_ptr<Region>>>> m_cells;


    //Initially is KEEP_GOING.
    SitRep m_sitRep{SitRep::KEEP_GOING};
    std::set<std::shared_ptr<Region>> m_regions;

    //This stores the output to be generated and converts into HTML.
    std::vector<std::tuple<std::string, std::vector<std::vector<State>>,
        set_pair_t, steady_clock_tp_t, int, set_pair_t>> m_output;


    std::mt19937 m_eng;


    bool analyze_complete_islands(bool const verbose);
    bool analyze_single_liberty(bool const verbose);
    bool analyze_dual_liberties(bool const verbose);
    bool analyze_unreachable_cells(bool const verbose);
    bool analyze_potential_pools(bool const verbose);
    bool analyze_confinement(bool const verbose, cache_map_t& cache);
    bool analyze_hypotheticals(bool const verbose);

    set_pair_t guessing_order();
    bool valid(int x, int y);
    State& cell(int x, int y);
    State const& cell(int x, int y) const;
    std::shared_ptr<Region>& region(int x, int y);
    std::shared_ptr<Region> const& region(int x, int y) const;

    void print(std::string_view s, set_pair_t const& updated = {},
               int failed_guesses = 0, set_pair_t const& failed_coords = {});

    bool process(bool const verbose, set_pair_t const& mark_as_black,
                 set_pair_t const& mark_as_white, std::string_view s,
                 int const failed_guesses = 0, set_pair_t const& failed_coords = {});

    template <typename F>
    void for_valid_neighbors(int x, int y, F&& f) const;

    void insert_valid_neighbors(set_pair_t& s, int x, int y) const;
    void insert_valid_unknown_neighbors(set_pair_t& s, int x, int y) const;

    void add_region(int x, int y);
    void mark(State const state, int x, int y);
    void fuse_regions(std::shared_ptr<Region> r1, std::shared_ptr<Region>r2);

    bool impossibly_big_white_region(int n) const;
    bool unreachable(int x_root, int y_root, set_pair_t discovered = {});
    bool confined(std::shared_ptr<Region>& r, cache_map_t& cache, set_pair_t const& verboten = {});

    bool detect_contradiction(bool verbose, cache_map_t& cache);



};
//Helper machinery for formatting time and print it to std::ostream.
std::string format_time(Grid::steady_clock_tp_t const start, Grid::steady_clock_tp_t const finish);


//Templates member functions definition.
template <typename It>
void Grid::Region::insert(It first, It last) {
    m_coords.insert(first, last);
}

template <typename It>
void Grid::Region::unk_insert(It first, It last) {
    m_unknowns.insert(first, last);
}

//Extract neighbors that are orthogonal and not
//off the  bound of the grid.
template <typename F>
void Grid::for_valid_neighbors(int x, int y, F&& f) const {

    if(x > 0) {
        f(std::forward<int>(x - 1), std::forward<int>(y));
    }
    if(x + 1 < m_width) {
        f(std::forward<int>(x + 1), std::forward<int>(y));
    }
    if(y > 0) {
        f(std::forward<int>(x), std::forward<int>(y - 1));
    }
    if(y + 1 < m_height){
        f(std::forward<int>(x), std::forward<int>(y + 1));
    }
}

}//End of nb namespace

#endif // GRID_HPP_INCLUDED

