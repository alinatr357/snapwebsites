// Snap Console -- two panel console with ncurses
// Copyright (C) 2018  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "snapwebsites/snap_console.h"

// our lib
//
#include "snapwebsites/not_used.h"

// C++ lib
//
#include <deque>
#include <iostream>

// C lib
//
#include <ncurses.h>
#include <readline/history.h>
#include <readline/readline.h>


namespace snap
{


/** \brief This is the actual implementation of the ncurses application.
 *
 * This class is what generates the two panels in the console and write
 * titles and handles the resize and input/output.
 *
 * The `snap_console` is the higher level user interface that allows you
 * to write to the console output. There is nothing you can do in the
 * input window.
 *
 * \todo
 * Later we'll add a statistics window so we can show various things
 * that we know of (i.e. number of messages, size transferred, etc.)
 *
 * \note
 * This class is very heavily based on a class written by ulfalizer
 * and found on github.com here:
 *
 * https://github.com/ulfalizer/readline-and-ncurses
 *
 * See also a post about this class on Stackoverflow.com:
 *
 * https://stackoverflow.com/questions/691652/using-gnu-readline-how-can-i-add-ncurses-in-the-same-program#28709979
 */
class ncurses_impl
{
public:
    typedef std::shared_ptr<ncurses_impl>    pointer_t;

    static pointer_t ptr()
    {
        if(f_snap_console->f_impl == nullptr)
        {
            ncurses_impl::fatal_error("ptr() called with f_snap_console->f_impl set to nullptr");
        }
        return f_snap_console->f_impl;
    }

    static ncurses_impl::pointer_t create_ncurses(snap_console * ce, std::string const & history_filename)
    {
        if(ce->f_impl == nullptr)
        {
            // can't use std::make_shared() as the constructor is private
            //
            ce->f_impl.reset(new ncurses_impl(ce, history_filename));

            pointer_t p(ptr());

            // we call the open from here instead of the constructor
            // because we need f_snap_console->f_impl to be defined for
            // fatal_error() to work properly
            //
            p->open_ncurse();
            p->open_readline();
            p->ready();
        }
        return ce->f_impl;
    }

    ~ncurses_impl()
    {
        // shouldn't these close() be called backward?
        //
        close_ncurse();
        close_readline();

        f_snap_console = nullptr;
    }

    bool process_read()
    {
        if(f_redisplay)
        {
            f_redisplay = false;
            win_input_redisplay(false);
        }

        while(!f_should_exit)
        {
            // Using getch() here instead would refresh stdscr, overwriting the
            // initial contents of the other windows on startup
            //
            int const c(wgetch(f_win_input));

//output("got [" + std::to_string(c) + "]");
            switch(c)
            {
            case ERR:
                // f_win_input is non-blocking, this happens when the
                // input buffer is empty and we are ready to return to
                // snap_communicator
                //
                return f_should_exit;

            // at this time handling ESC is "tough" because it happens
            // with many keys and since ncurses and readline are handling
            // things in some different ways, I'm not too sure where to
            // look at before to make it work properly...
            //
            //case '\033':
            //    f_should_exit = true;
            //    break;

            case KEY_RESIZE:
                resize();
                break;

            // Ctrl-L -- redraw screen
            case '\f':
                clear_output();
                break;

            default:
                forward_to_readline(c);
                break;

            }
        }

        return f_should_exit; // always true here at the moment
    }

    void output(std::string const & line)
    {
        pointer_t p(ptr());

        if(!f_first_line)
        {
            wprintw(f_win_output, "\n");
        }

        // save all the lines in f_output vector so we can redraw it in
        // case of a resize
        //
        // one day we may work on Page Up/Down to scroll through
        // this buffer too!
        //
        p->f_output.push_back(line);
        while(p->f_output.size() > 1000)
        {
            p->f_output.pop_front();
        }

        if(wprintw(f_win_output, "%s", line.c_str()) != OK)
        {
            fatal_error("wprintw() to output window failed");
        }
        if(wrefresh(p->f_win_output) != OK)
        {
            fatal_error("wrefresh() to output window failed");
        }
        f_first_line = false;
    }

    void clear_output()
    {
        // lose all output
        //
        f_output.clear();

        // makes the next refresh repaint the screen from scratch
        //
        if(clearok(curscr, TRUE) != OK)
        {
            fatal_error("clearok() failed in clear_output()");
        }

        // we will next be writing a first line again
        //
        f_first_line = true;

        // resize and reposition windows in case that got messed
        // up somehow
        //
        resize();
    }

    void set_prompt(std::string const & prompt)
    {
        rl_callback_handler_install(prompt.c_str(), got_command);
    }

private:
    void open_ncurse()
    {
        // setup locale
        //
        if(setlocale(LC_ALL, "") == nullptr)
        {
            fatal_error("Failed to set locale attributes from environment");
        }

        // initialize screen
        //
        f_win_main = initscr();
        if(f_win_main == nullptr)
        {
            fatal_error("initscr() failed to initialize ncurses");
        }

        // we've got a screen, we're in visual mode now
        //
        f_visual_mode = true;

        // initialize colors
        //
        if(has_colors())
        {
            if(start_color() != OK)
            {
                fatal_error("start_color() failed");
            }
            if(use_default_colors() != OK)
            {
                fatal_error("use_default_colors() failed");
            }
        }

        getmaxyx(f_win_main, f_screen_height, f_screen_width);
        if(f_screen_height < 5)
        {
            fatal_error("your console is not tall enough for this application");
        }

        if(cbreak() != OK)
        {
            fatal_error("cbreak() failed");
        }
        if(noecho() != OK)
        {
            fatal_error("noecho() failed");
        }
        if(nonl() != OK)
        {
            fatal_error("nonl() failed");
        }
        if(intrflush(nullptr, false) != OK)
        {
            fatal_error("intrflush() failed");
        }

        // IMPORTANT:
        // Do not enable keypad() as we want to pass unadulterated
        // input to readline()
        //
        // Only having keypad(win, TRUE) is the only way we can detect
        // whether the ESC key was used. I think the timeout is small
        // enough on a Linux box because it would be set to the minimum
        // of a keyboard repeat which is around 200ms.

        // Explicitly specify a "very visible" cursor to make sure it's
        // at least consistent when we turn the cursor on and off (maybe
        // it would make sense to query it and use the value we get back
        // too). "normal" vs. "very visible" makes no difference in
        // gnome-terminal or xterm. Let this fail for terminals that
        // do not support cursor visibility adjustments.
        //
        curs_set(2); // ignore errors

        draw_borders();

        // create two child windows
        //
        f_win_output = newwin(f_screen_height - 7, f_screen_width - 2, 1, 1);
        if(f_win_output == nullptr)
        {
            fatal_error("could not create output window");
        }

        f_win_input = newwin(4, f_screen_width - 2, f_screen_height - 5, 1);
        if(f_win_input == nullptr)
        {
            fatal_error("could not create input window");
        }

        // allow strings longer than the message window and show only the
        // last part if the string doesn't fit
        //
        if(scrollok(f_win_output, TRUE) != OK)
        {
            fatal_error("scrollok() failed; could not setup output window to scoll on large lines");
        }
        //if(scrollok(f_win_input, TRUE) != OK) -- TBD
        //{
        //    fatal_error("scrollok() failed; could not setup input window to scoll on large lines");
        //}

        // we want to make the wgetch() function non-blocking so that way
        // other things can happen
        //
        wtimeout(f_win_input, 0);

        // to make sure the cursor gets at the right place
        //
        f_redisplay = true;
    }

    void close_ncurse()
    {
        if(f_visual_mode)
        {
            if(f_win_output != nullptr)
            {
                delwin(f_win_output);
                f_win_output = nullptr;
            }

            if(f_win_input != nullptr)
            {
                delwin(f_win_input);
                f_win_input = nullptr;
            }

            // make sure endwin() is only called in visual mode.
            //
            // Note: calling it twice does not seem to be supported
            //       and messed with the cursor position.
            //
            endwin();

            f_visual_mode = false;
        }
    }

    void open_readline()
    {
        // disable auto-completion
        //
        if(rl_bind_key('\t', rl_insert) != 0)
        {
            fatal_error("invalid key passed to rl_bind_key()");
        }

        // TODO: allow for not using history
        //
        using_history();
        read_history(f_history_filename.c_str());

        // let ncurses do all terminal and signal handling
        //
        rl_catch_signals = 0;
        rl_catch_sigwinch = 0;
        rl_deprep_term_function = nullptr;
        rl_prep_term_function = nullptr;

        // prevent readline from setting the LINES and COLUMNS environment
        // variables, which override dynamic size adjustments in ncurses.
        // When using the alternate readline interface (as we do here),
        // LINES and COLUMNS are not updated if the terminal is resized
        // between two calls to rl_callback_read_char() (which is almost
        // always the case)
        //
        rl_change_environment = 0;

        // Handle input by manually feeding characters to readline
        // (TODO: save those pointers so the close_readline() can restore
        // what there wasthere instead of assuming the defaults.)
        //
        f_has_handlers = true;
        rl_getc_function = readline_getc;
        rl_input_available_hook = readline_input_avail;
        rl_redisplay_function = readline_redisplay;

        set_prompt("> ");
    }

    void close_readline()
    {
        if(f_has_handlers)
        {
            rl_getc_function = rl_getc;
            rl_input_available_hook = nullptr;
            rl_redisplay_function = rl_redisplay;
            rl_callback_handler_remove();
            f_has_handlers = false;
        }
    }

    void ready()
    {
        output("Ready.\nType /help for help screen.");
    }

    void draw_borders()
    {
        // setup the background window with borders and names
        //
        wborder(f_win_main, 0, 0, 0, 0, 0, 0, 0, 0);
        mvwaddch(f_win_main, f_screen_height - 6, 0, ACS_LTEE);
        mvwhline(f_win_main, f_screen_height - 6, 1, ACS_HLINE, f_screen_width - 2);
        mvwaddch(f_win_main, f_screen_height - 6, f_screen_width - 1, ACS_RTEE);
        mvwprintw(f_win_main, 0, 2, " Output ");
        mvwprintw(f_win_main, f_screen_height - 6, 2, " Console (Ctrl-D on empty line to exit) ");
        wrefresh(f_win_main);
    }

    static int readline_getc(FILE * dummy)
    {
        pointer_t p(ptr());

        snap::NOTUSED(dummy);

        p->f_input_available = 0; // false

        return p->f_input;
    }

    void forward_to_readline(int c)
    {
        // extend character without sign
        //
        f_input = c;

        f_input_available = 1; // true

        rl_callback_read_char();
    }

    static int readline_input_avail()
    {
        pointer_t p(ptr());
        return p->f_input_available;
    }

    static void readline_redisplay()
    {
        pointer_t p(ptr());
        p->win_input_redisplay(false);
    }

    static void got_command(char * line)
    {
        pointer_t p(ptr());

        p->f_redisplay = true;

        if(line == nullptr)
        {
            // Ctrl-D pressed on empty line
            //
            p->f_should_exit = true;

            f_snap_console->process_quit();
        }
        else
        {
            std::string l(line);
            if(!l.empty())
            {
                // add to history
                //
                add_history(line);
                write_history(p->f_history_filename.c_str());

                p->output(l);

                f_snap_console->process_command(l);
            }
            free(line);

            p->win_input_redisplay(false);
        }
    }

    void win_output_redisplay(bool for_resize)
    {
        if(werase(f_win_output) != OK)
        {
            fatal_error("werase() of output window failed");
        }

        draw_borders();

        // redraw the output buffer
        //
        // DO NOT USE the output() function for a few reasons:
        //
        //   1. it will call wrefresh() on each call (argh!)
        //   2. it will re-add the buffer to itself
        //   3. the change of f_oupt may crash the for() loop
        //
        char const * nl = "";
        for(auto const & l : f_output)
        {
            if(wprintw(f_win_output, "%s%s", nl, l.c_str()) != OK)
            {
                fatal_error("wprintw() to output window failed");
            }
            nl = "\n";
        }

        // We batch window updates when resizing
        //
        if(for_resize)
        {
            if(wnoutrefresh(f_win_output) != OK)
            {
                fatal_error("wnoutrefresh() of output window failed");
            }
        }
        else
        {
            if(wrefresh(f_win_output) != OK)
            {
                fatal_error("wrefresh() of output window failed");
            }
        }
    }

    /** \brief Redraw the input window.
     *
     * Each time the user enters a character on the keyboard this
     * function gets called. It will redraw the input window to its
     * current state.
     *
     * By default the function will update the screen with a call
     * to wrefresh(). If you set the for_resize flag to true, then it
     * calls wnoutrefresh() instead, which marks the window for refresh
     * but does not refresh it right away.
     *
     * The function also positions the cursor.
     *
     * \param[in] for_resize  Whether this is called by the resize or not.
     */
    void win_input_redisplay(bool for_resize)
    {
        // WARNING: we have two test because if the prompt includes a
        //          tab then the size is different than without a tab
        //          in there (at least that's the only thing that could
        //          affect the calculation done in strnwidth())
        //
        size_t const prompt_width = strnwidth(rl_display_prompt, SIZE_MAX, 0);
        size_t const cursor_col = prompt_width +
                            strnwidth(rl_line_buffer, rl_point, prompt_width);

        if(werase(f_win_input) != OK)
        {
            fatal_error("werase() failed");
        }

        // this might write a string wider than the terminal currently,
        // so don't check for errors
        //
        mvwprintw(f_win_input, 0, 0, "%s%s", rl_display_prompt, rl_line_buffer);

        int x(cursor_col % (f_screen_width - 2));
        int y(cursor_col / (f_screen_width - 2));
        if(y >= 4)
        {
            // hide the cursor if it lies outside the window
            // otherwise it breaks the wmove() call
            //
            curs_set(0);
        }
        else
        {
            if(wmove(f_win_input, y, x) != OK)
            {
                fatal_error("wmove() failed");
            }
            curs_set(2);
        }

        // we batch window updates when resizing
        //
        if(for_resize)
        {
            if(wnoutrefresh(f_win_input) != OK)
            {
                fatal_error("wnoutrefresh() failed");
            }
        }
        else
        {
            if(wrefresh(f_win_input) != OK)
            {
                fatal_error("wrefresh() failed");
            }
        }
    }

    /** \brief We got a resize signal, make sure to redraw everything.
     *
     * Whenever the user resizes his console, this function gets called
     * to resize the windows and move them around as required. The
     * function updates the screen width and height so all the other
     * functions don't have to read those two parameters over and
     * over again.
     */
    void resize()
    {
        // get the new width and height of the screen
        //
        getmaxyx(f_win_main, f_screen_height, f_screen_width);

        if(f_screen_height < 5)
        {
            fatal_error("window too small after resize");
        }

        if(wresize(f_win_output, f_screen_height - 7, f_screen_width - 2) != OK)
        {
            fatal_error("wresize of output window failed");
        }
        if(wresize(f_win_input, 4, f_screen_width - 2) != OK)
        {
            fatal_error("wresize of input window failed");
        }

        if(mvwin(f_win_input, f_screen_height - 5, 1) != OK)
        {
            fatal_error("mvwin of input window failed");
        }

        // batch refreshes and commit them with doupdate()
        win_output_redisplay(true);
        win_input_redisplay(true);

        if(doupdate() != OK)
        {
            fatal_error("doupdate() after wresize() failed");
        }
    }

    /** \brief End this software with an error.
     *
     * This function is expected to close the ncurse screen and then
     * write an error message in the output before exiting with 1.
     *
     * \param[in] msg  The error message to display.
     */
    [[noreturn]] static void fatal_error(char const * msg)
    {
        // don't use ptr() since it calls fatal_error() if the
        // pointer is nullptr
        //
        if(f_snap_console->f_impl != nullptr)
        {
            f_snap_console->f_impl->close_ncurse();
            f_snap_console->f_impl.reset();
        }
        std::cerr << msg << std::endl;
        exit(1);
    }

    /** \brief Calculate the width of a string.
     *
     * This function is used to calculate the cursor position.
     *
     * The function knows how to calculate the width of any character:
     *
     * \li multi-byte
     * \li multi-column
     * \li combining
     *
     * Unfortunately, this is a copy of the readline() function which is
     * not being exported so we do not have access to it and have had to
     * rewrite it here.
     *
     * The function returns the total width in columns of the string.
     * The `n` parameter can be used to limit the number of characters
     * to check. The `offset` can be used to ignore a certain number
     * of characters at the start.
     *
     * \note
     * The function will makes a guess for malformed strings (strings with
     * invalid multi-byte characters).
     *
     * \param[in] s  The string for which we calculate the width.
     * \param[in] n  The maximum number of characters to count or SIZE_MAX.
     * \param[in] offset The offset at which to start calculating the size.
     */
    size_t strnwidth(char const * s, size_t n, size_t offset)
    {
        mbstate_t shift_state = mbstate_t();

        // Start in the initial shift state
        //memset(&shift_state, '\0', sizeof(shift_state));

        size_t width(0);
        size_t wc_len(0);
        for(size_t i(0); i < n; i += wc_len)
        {
            // Extract the next multibyte character
            wchar_t wc(0);
            wc_len = mbrtowc(&wc, s + i, MB_CUR_MAX, &shift_state);
            switch(wc_len)
            {
            case 0:
                // Reached the end of the string
                goto done;

            case static_cast<size_t>(-1):
            case static_cast<size_t>(-2):
                // Failed to extract character. Guess that each character is one
                // byte/column wide each starting from the invalid character to
                // keep things simple.
                width += strnlen(s + i, n - i);
                goto done;

            }

            if (wc == '\t')
            {
                width = ((width + offset + 8) & ~7) - offset;
            }
            else
            {
                // TODO: readline also outputs ~<letter> and the like for some
                //       non-printable characters
                //
                width += iswcntrl(wc) ? 2 : std::max(0, wcwidth(wc));
            }
        }

    done:
        return width;
    }

private:
    /** \brief Initialize the ncurses_impl object.
     *
     * This function saves the `snap_console` pointer to the static
     * pointer defined here.
     *
     * Note that you can't have more than one `ncurses_impl` at a time
     * (not just because of that static, trust me!) The constructor
     * of the `create_ncurses()` function makes sure of that, although
     * it is not tested properly at this point.
     *
     * \note
     * I use a bare pointer because it is a one to one relationship
     * like a parent (`snap_console`) and a child (`ncurses_impl`).
     * If that goes wrong, then we've got a much better problem.
     *
     * \param[in] ce  The `snap_console` pointer.
     */
    ncurses_impl(snap_console * ce, std::string const & history_filename)
    {
        f_snap_console = ce;

        // keep the default is not specified
        //
        if(!history_filename.empty())
        {
            f_history_filename = history_filename;
        }

        // if it starts with "~/", consider changing the "~" with "$HOME"
        //
        if(f_history_filename.length() > 1
        && f_history_filename[0] == '~'
        && f_history_filename[1] == '/')
        {
            char * home = getenv("HOME");
            if(home != nullptr
            && *home != '\0')
            {
                // replace the '~' with the $HOME contents
                //
                f_history_filename = home + f_history_filename.substr(1);
            }
        }

        // WARNING: our initialization required f_snap_console to be defined
        //          so better not have anything in the constructor
        //          at this point...
    }

    //static ncurses_impl::pointer_t  f_nc;
    static snap_console *           f_snap_console; // initialized below (because it is static)
    std::string                     f_history_filename = "~/.snap_history";
    WINDOW *                        f_win_main = nullptr;
    WINDOW *                        f_win_output = nullptr;
    WINDOW *                        f_win_input = nullptr;
    int                             f_screen_width = 0;
    int                             f_screen_height = 0;
    std::deque<std::string>         f_output;
    bool                            f_visual_mode = false;
    bool                            f_has_handlers = false;
    bool                            f_should_exit = false;
    bool                            f_first_line = true;
    bool                            f_redisplay = true;
    int                             f_input_available = 0;
    int                             f_input = 0;
};

//ncurses_impl::pointer_t   ncurses_impl::f_nc;
snap_console *     ncurses_impl::f_snap_console = nullptr;







snap_console::snap_console(std::string const & history_filename)
    : snap_fd_connection(fileno(stdin), mode_t::FD_MODE_READ)
{
    f_impl = ncurses_impl::create_ncurses(this, history_filename);
}

snap_console::~snap_console()
{
    f_impl.reset();
}

void snap_console::output(std::string const & line)
{
    f_impl->output(line);
}

void snap_console::clear_output()
{
    f_impl->clear_output();
}

void snap_console::set_prompt(std::string const & prompt)
{
    f_impl->set_prompt(prompt);
}

void snap_console::process_read()
{
    if(f_impl->process_read())
    {
        // we're done, user hit Ctrl-D or /quit
        //
        mark_done();
    }
}

}
// namespace snap

// vim: ts=4 sw=4 et