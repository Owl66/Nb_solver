#include "Grid.hpp"

#include <sstream>
#include <regex>
#include <string>
#include <cassert>






namespace nb_s {


Grid::Grid(int const width, int const height, std::string s)
    : m_width{width}
    , m_height{height}
    , m_total_black{width * height}
    , m_cells{}
    , m_sitRep{SitRep::KEEP_GOING}
    , m_regions{}
    , m_output{}
    , m_string{std::move(s)}
    , m_eng{1729} {



    if(width < 1) {
        throw std::runtime_error("width should be greater than 1: Grid::Grid()");
    }
    if(height < 1) {
        throw std::runtime_error("height should be greater than 1: Grid::Grid()");
    }

    m_cells.resize(width, std::vector<std::pair<State, std::shared_ptr<Region>>>
                   (height, std::make_pair(State::UNKNOWN, std::shared_ptr<Region>())));

    //Parse the string.
    std::vector<int> parser;
    std::regex const rx{R"((\d+)|( )|(\n)|[^\d \n])"};

    for(sregex_iterator i{m_grid.begin(), m_grid.end(), rx}, end; i != end; ++i) {

        auto const& m = *i;
        if(m[1].matched) {
            parser.push_back(std::stoi(m[1]));

        } else if(m[2].matched) {
            parser.push_back(0);

        } else if(m[3].matched) {
            //do nothing.

        } else {
            throw std::runtime_error("Grid::Grid(): Grid initialization contains invalid string.");
        }
    }

    if(parser.size() != static_cast<size_t>(width * height)) {
        throw std::runtime_error("Grid::Grid(): grid must contain "
                                 "string must contain width * height spaces and numbers");
    }

    for(auto i = 0; i < width; ++i) {
        for(auto j = 0; j < height; ++j) {
            auto const n = v[x + y * width];

            if(n > 0) {


            }
            if(valid(x, y - 1) && cell(x, y - 1) > 0) {
                throw std::runtime_error("Grid::Grid() the vertical adjacent cells "
                                         "string cannot be numbered.");
            }

            cell(x, y) = static_cast<State>(n);
            add_region(x, y);

            //Get the total number of black cells in the grid.
            m_total_black -= n;
        }

    print("I'm okay to go!");
}

Grid::SitRep Grid::solve(bool verbose, bool guessing) {

	cache_map_t cache;

	if(known() == m_width * m_height) {
		if(detect_contradictions(verbose, cache)) {
			return SitRep::CONTRADICTION_FOUND;
		}

		if(verbose) {
			print("I'm done!");
		}

		return SitRep::SOLUTION_FOUND;
	}
	if(analyze_complete_islands(verbose)
		|| analyze_single_liberties(verbose)
		|| analyze_dual_liberties(verbose)
		|| analyze_unreachable_cells(verbose)
		|| analyze_potential_pools(verbose)
		|| detect_contradictions(verbose, cache)
		|| analyze_confinement(verbose, cache)
		|| (guessing && analyze_hypotheticals(verbose))) {

			return m_sitRep
		}
	if(verbose) {
		print("I'm stumped!")
	}

	return SitRep::CANNOT_PROCEED;
}

int Grid::known() const {

}

void Grid::write(std::ostream& os, steady_clock_tp start, steady_clock_tp finish) const {

}


namespace detail {


Grid::Region::Region(State const state, set_pair_t const& unknowns, int const x, int const y)
    : m_state{state}

      //Tracks the region.
    , m_coords{}

      //Tracks unknown cells surround the region.
    , m_unknows{unknowns} {


    assert(state != State::UNKNOWN);

    //create the region.
    m_coords.insert(std::make_pair(x, y));
}



bool Grid::Region::is_white() const {
    return m_state == State::WHITE;
}

bool Grid::Region::is_black() const {
    return m_state == State::BLACK;
}

bool Grid::Region::is_numbered() const {
    return m_state > 0;
}

int Grid::Region::number() const {
    assert(is_numbered);
    return m_state;
}

Grid::set_pair_t::const_iterator Grid::Region::begin() const {
    return m_coords.begin();
}

Grid::set_pair_t::const_iterator Grid::Region::end() const {
    return m_coords.end();
}

int Grid::Region::size() const {
    return static_cast<int>(m_coords.size());
}

bool Grid::Region::contains(int const x, int const y) const {
    return m_coords.find(std::make_pair(x, y)) != m_coords.end();
}


Grid::set_pair_t::const_iterator Grid::Region::unk_begin() const {
    return m_unknowns.begin();
}

Grid::set_pair_t::const_iterator Grid::Region::unk_end() const {
    return m_unknowns.end();
}


int Grid::Region::unk_size() const {
    return static_cast<int>(m_unknowns.size());
}

void Grid::Region::unk_erase(int const x, int const y) {
    m_unknowns.erase(std::make_pair(x, y));
}
}//End of detail namespace.


bool Grid::analyze_complete_islands(bool verbose) {

}

bool Grid::analyze_single_liberty(bool verbose) {

}

bool Grid::analyze_dual_liberties(bool verbose) {

}

bool Grid::analyze_unreachable_cells(bool verbose) {

}

bool Grid::analyze_potential_pools(bool verbose) {

}

bool Grid::analyze_confinement(bool verbose, cache_map_t& cache) {

}

bool Grid::analyze_hypotheticals(bool verbose) {

}

Grid::set_pair_t Grid::guessing_order() {

}

bool Grid::valid(int x, int y) {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

Grid::State const& Grid::cell(int x, int y) const {
    return m_cells[x][y].first;
}

Grid::State& Grid::cell(int x, int y) {
    return m_cells[x][y].first;
}

std::shared_ptr<Grid::detail::Region>& Grid::detail::region(int x, int y) {
    return m_cells[x][y].second;
}

std::shared_ptr<Grid::detail::Region> const& Grid::detail::region(int x, int y) const {
    return m_cells[x][y].second;
}

void Grid::print(std::string const& s, set_pair_t const& updated, int failed_guesses, set_pair_t const& failed_coords) {

}

bool Grid::process(bool verbose, set_pair_t const& mark_as_black, set_pair_t const& mark_as_white, std::string const& s) {

}



void Grid::insert_valid_neighbors(set_pair_t& s, int x, int y) const {

}

void Grid::insert_valid_unknown_neighbors(set_pair_t& s, int x, int y) const {

}

void Grid::add_region(int x, int y) {

}

void Grid::mark(int x, int y) {

}

void Grid::fuse_regions(std::shared_ptr<detail::Region>r1, std::shared_ptr<detail::Region>r2) {

}

bool Grid::impossibly_big_white_region(int n) const {

}

bool Grid::unreachable(int x_root, int y_root, set_pair_t discovered) {

}

bool Grid::confined(std::shared_ptr<detail::Region>& r, cache_map_t& cache, set_pair_t const& verboten) {

}

bool Grid::detect_contradiction(bool verbose, cache_map_t& cache) {

}

std::string format_time(Grid::steady_clock_tp const start, Grid::steady_clock_tp const finish) {

    std::ostringstream ostream;
    using namespace std::literals::chrono_literals;

    if(finish - start < 1s) {
        ostream << std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(finish - start).count()
                << " milliseconds" ;

    } else if(finish - start < 1ms) {
        ostream << std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(finish - start).count()
                << " microseconds";

    } else {
        ostream << std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count()
                << " seconds";
    }

    return ostream.str();

}
}// end of namespace nb_s
