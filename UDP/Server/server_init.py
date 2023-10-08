import ncurses

def main():
    # Initialize ncurses
    ncurses.initscr()

    # Create a new window
    window = ncurses.newwin(10, 20, 0, 0)

    # Add a label to the window
    label = ncurses.newwin(1, 10, 0, 0)
    label.addstr("Hello, world!")

    # Refresh the window
    window.refresh()

    # Start the event loop
    ncurses.getch()

    # End ncurses
    ncurses.endwin()

if __name__ == "__main__":
    main()
