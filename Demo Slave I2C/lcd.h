//headers for LCD functions 
//Roger Traylor 4.26.07

void strobe_lcd(void);
void clear_display(void);
void cursor_home(void);
void cursor_home_nodelay(void);
void home_line2(void);
void home_line2_nodelay(void);      
void fill_spaces(void);
void char2lcd(char a_char);
void char2lcd_nodelay(char a_char);
void string2lcd(char *lcd_str);
void lcd_init(void);
