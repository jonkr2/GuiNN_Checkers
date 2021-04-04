void reg_set_int(char *name, int val);
int reg_get_int(char *name, int *val);
void reg_set_string(char *name, char *string);
int reg_get_string(char *name, char *string, int maxlen);
void save_hashsize(int val);
void save_book_setting(int val);
void save_dbpath(char *path);
void save_enable_wld(int enable_wld);
void save_dbmbytes(int val);
void save_max_dbpieces(int val);
int get_hashsize(int *size);
int get_book_setting(int *setting);
int get_dbpath(char *path, int maxlen);
int get_enable_wld(int *enable_wld);
int get_dbmbytes(int *size);
int get_max_dbpieces(int *size);


