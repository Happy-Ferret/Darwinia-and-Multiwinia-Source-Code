/*
 *  $Id: checklist.c,v 1.7 2004/12/15 05:04:47 megastep Exp $
 *
 *  checklist.c -- implements the checklist box
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *     Stuart Herbert - S.Herbert@sheffield.ac.uk: radiolist extension
 *     Alessandro Rubini - rubini@ipvvis.unipv.it: merged the two
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "dialog.h"

static int list_width, check_x, item_x, checkflag;

#define LLEN(n) ((n) * CHECKBOX_TAGS)
#define ItemData(i)    &items[LLEN(i)]
#define ItemName(i)    items[LLEN(i)]
#define ItemText(i)    items[LLEN(i) + 1]
#define ItemStatus(i)  items[LLEN(i) + 2]
#define ItemHelp(i)    items[LLEN(i) + 3]

/*
 * Print list item
 */
static void
print_item(WINDOW *win, const char **items, int status,
	   int choice, int selected)
{
    int i;

    /* Clear 'residue' of last item */
    wattrset(win, menubox_attr);
    (void) wmove(win, choice, 0);
    for (i = 0; i < list_width; i++)
	(void) waddch(win, ' ');

    (void) wmove(win, choice, check_x);
    wattrset(win, selected ? check_selected_attr : check_attr);
    (void) wprintw(win,
		   (checkflag == FLAG_CHECK) ? "[%c]" : "(%c)",
		   status ? 'X' : ' ');
    wattrset(win, menubox_attr);
    (void) waddch(win, ' ');
    wattrset(win, selected ? tag_key_selected_attr : tag_key_attr);
	if ( ItemName(0)[0] ) {
		(void) waddch(win, CharOf(ItemName(0)[0]));
		wattrset(win, selected ? tag_selected_attr : tag_attr);
		(void) wprintw(win, "%.*s", item_x - check_x - 6, ItemName(0) + 1);
	}

    (void) wmove(win, choice, item_x);
    wattrset(win, selected ? item_selected_attr : item_attr);
    (void) wprintw(win, "%.*s", getmaxx(win) - item_x, ItemText(0));

    if (selected) {
		dlg_item_help(ItemHelp(0));
    }
}

/*
 * Display a dialog box with a list of options that can be turned on or off
 * The `flag' parameter is used to select between radiolist and checklist.
 */
int
dialog_checklist(const char *title, const char *cprompt, int height, int width,
				 int list_height, int item_no, const char **items, int flag,
				 int *results)
{
    int i, j, found, x, y, cur_x, cur_y, box_x, box_y;
    int key = 0, button = 0, choice = 0, scrollamt = 0, max_choice, *status;
    int use_width, name_width, text_width;
    int done = FALSE, result = DLG_EXIT_OK;
    WINDOW *dialog, *list;
    char *prompt = strclone(cprompt);
    const char **buttons = dlg_ok_labels();

    tab_correct_str(prompt);
    if (list_height == 0) {
		use_width = calc_listw(item_no, items, CHECKBOX_TAGS) + 10;
		/* calculate height without items (4) */
		auto_size(title, prompt, &height, &width, 4, MAX(26, use_width));
		calc_listh(&height, &list_height, item_no);
    } else {
		auto_size(title, prompt, &height, &width, 4 + list_height, 26);
    }
    print_size(height, width);
    ctl_size(height, width);

    checkflag = flag;

    /* Allocate space for storing item on/off status */
    if ((status = malloc(sizeof(int) * item_no)) == NULL)
		exiterr("Can't allocate memory in dialog_checklist().");

    /* Initializes status */
    for (i = 0; i < item_no; i++)
		status[i] = !dlg_strcmp(ItemStatus(i), "on");

    max_choice = MIN(list_height, item_no);

    x = box_x_ordinate(width);
    y = box_y_ordinate(height);

    dialog = new_window(height, width, y, x);

    mouse_setbase(x, y);

    draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
    draw_bottom_box(dialog);
    draw_title(dialog, title);

    wattrset(dialog, dialog_attr);
    print_autowrap(dialog, prompt, height, width);

    list_width = width - 6;
    getyx(dialog, cur_y, cur_x);
    box_y = cur_y + 1;
    box_x = (width - list_width) / 2 - 1;

    /* create new window for the list */
    list = sub_window(dialog, list_height, list_width,
					  y + box_y + 1, x + box_x + 1);

    /* draw a box around the list items */
    draw_box(dialog, box_y, box_x,
			 list_height + 2 * MARGIN,
			 list_width + 2 * MARGIN,
			 menubox_border_attr, menubox_attr);

    text_width = 0;
    name_width = 0;
    /* Find length of longest item to center checklist */
    for (i = 0; i < item_no; i++) {
		text_width = MAX(text_width, (int) strlen(ItemText(i)));
		name_width = MAX(name_width, (int) strlen(ItemName(i)));
    }

    /* If the name+text is wider than the list is allowed, then truncate
     * one or both of them.  If the name is no wider than 1/4 of the list,
     * leave it intact.
     */
    use_width = (list_width - 6);
    if (text_width + name_width > use_width) {
		int need = 0.25 * use_width;
		if (name_width > need) {
			int want = use_width * ((double) name_width) / (text_width + name_width);
			name_width = (want > need) ? want : need;
		}
		text_width = use_width - name_width;
    }

    check_x = (use_width - (text_width + name_width)) / 2;
    item_x = name_width + check_x + 6;

    /* Print the list */
    for (i = 0; i < max_choice; i++)
		print_item(list,
				   ItemData(i),
				   status[i], i, i == choice);
    (void) wnoutrefresh(list);

    /* register the new window, along with its borders */
    mouse_mkbigregion(box_y, box_x, list_height + 2, list_width + 2,
					  item_no, item_x,	/* the threshold */
					  0 /* normal */ );

    dlg_draw_arrows(dialog, scrollamt,
					scrollamt + choice < item_no - 1,
					box_x + check_x + 5,
					box_y,
					box_y + list_height + 1);

    dlg_draw_buttons(dialog, height - 2, 0, buttons, 0, FALSE, width);

    wtimeout(dialog, WTIMEOUT_VAL);

    while (!done) {
		key = mouse_wgetch(dialog);

		/*
		 * A space toggles the item status.  We handle either a checklist
		 * (any number of items can be selected) or radio list (zero or one
		 * items can be selected).
		 */
		if (key == ' ') {
			getyx(dialog, cur_y, cur_x);
			if (flag == FLAG_CHECK) {	/* checklist? */
				status[scrollamt + choice] = !status[scrollamt + choice];
				print_item(list,
						   ItemData(scrollamt + choice),
						   status[scrollamt + choice], choice, TRUE);
			} else {		/* radiolist */
				if (!status[scrollamt + choice]) {
					for (i = 0; i < item_no; i++)
						status[i] = FALSE;
					status[scrollamt + choice] = TRUE;
					for (i = 0; i < max_choice; i++)
						print_item(list,
								   ItemData(scrollamt + i),
								   status[scrollamt + i], i, i == choice);
				}
			}
			(void) wnoutrefresh(list);
			(void) wmove(dialog, cur_y, cur_x);
			wrefresh(dialog);
			continue;		/* wait for another key press */
		}

		/*
		 * Check if key pressed matches first character of any item tag in
		 * list.  If there is more than one match, we will cycle through
		 * each one as the same key is pressed repeatedly.
		 */
		found = FALSE;
		for (j = scrollamt + choice + 1; j < item_no; j++) {
			if (toupper(key) ==
				toupper(ItemName(j)[0])) {
				found = TRUE;
				i = j - scrollamt;
				break;
			}
		}
		if (!found) {
			for (j = 0; j <= scrollamt + choice; j++) {
				if (toupper(key) ==
					toupper(ItemName(j)[0])) {
					found = TRUE;
					i = j - scrollamt;
					break;
				}
			}
		}

		/*
		 * A single digit (1-9) positions the selection to that line in the
		 * current screen.
		 */
		if (!found
			&& (key <= '9')
			&& (key > '0')
			&& (key - '1' <= max_choice)) {
			found = TRUE;
			i = key - '1';
		}

		if (!found) {
			found = TRUE;
			switch (key) {
			case KEY_HOME:
				i = -scrollamt;
				break;
			case KEY_LL:
			case KEY_END:
				i = item_no - 1 - scrollamt;
				break;
			case KEY_PPAGE:
				if (choice)
					i = 0;
				else if (scrollamt != 0)
					i = -MIN(scrollamt, max_choice);
				else
					continue;
				break;
			case KEY_NPAGE:
				i = MIN(choice + max_choice, item_no - scrollamt - 1);
				break;
			case KEY_UP:
			case '-':
				i = choice - 1;
				if (choice == 0 && scrollamt == 0)
					continue;
				break;
			case KEY_DOWN:
			case '+':
				i = choice + 1;
				if (scrollamt + choice >= item_no - 1)
					continue;
				break;
			default:
				found = FALSE;
				break;
			}
		}

		if (found) {
			if (i != choice) {
				getyx(dialog, cur_y, cur_x);
				if (i < 0 || i >= max_choice) {
#if 0
#if defined(NCURSES_VERSION_MAJOR) && NCURSES_VERSION_MAJOR < 5
					/*
					 * Using wscrl to assist ncurses scrolling is not needed
					 * in version 5.x
					 */
					if (i == -1) {
						if (list_height > 1) {
							/* De-highlight current first item */
							print_item(list,
									   ItemData(scrollamt),
									   status[scrollamt], 0, FALSE);
							scrollok(list, TRUE);
							wscrl(list, -1);
							scrollok(list, FALSE);
						}
						scrollamt--;
						print_item(list,
								   ItemData(scrollamt),
								   status[scrollamt], 0, TRUE);
					} else if (i == max_choice) {
						if (list_height > 1) {
							/* De-highlight current last item before scrolling up */
							print_item(list,
									   ItemData(scrollamt + max_choice - 1),
									   status[scrollamt + max_choice - 1],
									   max_choice - 1, FALSE);
							scrollok(list, TRUE);
							wscrl(list, 1);
							scrollok(list, FALSE);
						}
						scrollamt++;
						print_item(list,
								   ItemData(scrollamt + max_choice - 1),
								   status[scrollamt + max_choice - 1],
								   max_choice - 1, TRUE);
					} else
#endif
						{
							if (i < 0) {
								scrollamt += i;
								choice = 0;
							} else {
								choice = max_choice - 1;
								scrollamt += (i - max_choice + 1);
							}
							for (i = 0; i < max_choice; i++) {
								print_item(list,
										   ItemData(scrollamt + i),
										   status[scrollamt + i], i, i == choice);
							}
						}
					(void) wnoutrefresh(list);
					dlg_draw_arrows(dialog, scrollamt,
									scrollamt + choice < item_no - 1,
									box_x + check_x + 5,
									box_y,
									box_y + list_height + 1);
#endif
				} else {
					/* De-highlight current item */
					print_item(list,
							   ItemData(scrollamt + choice),
							   status[scrollamt + choice], choice, FALSE);
					/* Highlight new item */
					choice = i;
					print_item(list,
							   ItemData(scrollamt + choice),
							   status[scrollamt + choice], choice, TRUE);
					(void) wnoutrefresh(list);
					(void) wmove(dialog, cur_y, cur_x);
					wrefresh(dialog);
				}
			}
			continue;		/* wait for another key press */
		}

		switch (key) {
		case M_EVENT + 0:
			done = TRUE;
			break;
		case '\n':
			done = TRUE;
			result = button ? DLG_EXIT_CANCEL : DLG_EXIT_OK;
			break;
		case M_EVENT + 1:
			result = DLG_EXIT_CANCEL;
			done = TRUE;
			break;
		case TAB:
		case KEY_LEFT:
		case KEY_RIGHT:
			if (!dialog_vars.nocancel)
				button = !button;
			dlg_draw_buttons(dialog, height - 2, 0, buttons, button, FALSE, width);
			break;
		case ESC:
			result = DLG_EXIT_ESC;
			done = TRUE;
			break;
		}
    }

    del_window(dialog);
    if (result == DLG_EXIT_OK) {
		for (i = 0; i < item_no; i++) {
			results[i] = status[i] ? TRUE : FALSE;
		}
    }
    free(status);
    return result;
}
