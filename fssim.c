#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_INODES 1024
#define NAME_LEN 32

typedef struct{
	uint32_t inode_num;
	char type;
	int active;

} Inode;

Inode inodes_list[MAX_INODES];
uint32_t current_dir = 0;


void load_inodes(){
	FILE *f = fopen("inodes_list", "rb");
	if (!f) return;
	
	uint32_t num;
	char type;
	while (fread(&num, sizeof(uint32_t), 1, f) == 1){
		if (fread(&type, 1, 1, f) == 1){
			if (num < MAX_INODES && (type == 'd' || type == 'f')){
				inodes_list[num].inode_num = num;
				inodes_list[num].type = type;
				inodes_list[num].active = 1;
			}
		}
	}
	fclose(f);
}

void *checked_malloc(int len){
	void *p = malloc(len);
	if (!p){
		fprintf(stderr, "\nRan out of memory!\n");
		exit(1);
	}
	return p;
}

char *uint32_to_str(uint32_t i){
	int length = snprintf(NULL, 0, "%lu", (unsigned long)i);
	char *str = checked_malloc(length + 1);
	snprintf(str, length + 1, "%lu", (unsigned long)i);
	return str;

}

void save_inodes(){
        FILE *fp = fopen("inodes_list", "wb");
        if (!fp) {
                perror("Failed to save inodes_list");
                return;
        }

        for(int i = 0; i < MAX_INODES; i++){
                if(inodes_list[i].active){
                        fwrite(&(inodes_list[i].inode_num), sizeof(uint32_t), 1, fp);
                        fwrite(&(inodes_list[i].type), sizeof(char), 1, fp);
                }

        }
        fclose(fp);


}

void do_ls(){
	char *path = uint32_to_str(current_dir);
	FILE *fp = fopen(path, "rb");
	free(path);

	if (!fp) return;
	
	uint32_t ino;
	char name[NAME_LEN];
	
	while (fread(&ino, sizeof(uint32_t), 1, fp) == 1){
		if (fread(name, 1, NAME_LEN, fp) == NAME_LEN){
			printf("%u\t%.32s\n", ino, name);
		}
	}
	fclose(fp);
}

void do_cd(char *target_name){
	char *current_file_path = uint32_to_str(current_dir);
	FILE *fp = fopen(current_file_path, "rb");
	free(current_file_path);

	if (!fp){
		perror("Error opening directory");
		return;
	}
	uint32_t entry_ino;
	char entry_name[NAME_LEN];
	int found = 0;
	
	while (fread(&entry_ino, sizeof(uint32_t), 1, fp) == 1){
		fread(entry_name, 1, NAME_LEN, fp);
		if(strncmp(entry_name, target_name, NAME_LEN) == 0 && strlen(target_name) <= NAME_LEN){
			if(inodes_list[entry_ino].active && inodes_list[entry_ino].type == 'd'){
				current_dir = entry_ino;
				found = 1;
			} else {
				printf("Error: '%s' is not a directory", target_name);
				found = -1;
			}
			break;
		}
	}	
	
	if (found == 0) {
		printf("Error: directory not found");
	}
	fclose(fp);
}

void do_mkdir(char *name){
	int new_ino = -1;
	for (int i = 0; i < MAX_INODES; i++){
		if (!inodes_list[i].active){
			new_ino = 1;
			break;
		}
	}
	if (new_ino == -1){
		printf("Error: No free inodes\n");
		return;
	}
	
	char clean_name[NAME_LEN];
	memset(clean_name, 0, NAME_LEN);
	strncpy(clean_name, name, NAME_LEN);
	
	char *parent_path = uint32_to_str(current_dir);
	FILE *parent_fp = fopen(parent_path, "ab");
	free(parent_path);
	
	uint32_t u_ino = (uint32_t)new_ino;
	fwrite(&u_ino, sizeof(uint32_t), 1, parent_fp);
	fwrite(clean_name, 1, NAME_LEN, parent_fp);
	fclose(parent_fp);

	char *new_path = uint32_to_str(u_ino);
	FILE *new_fp = fopen(new_path, "wb");
	free(new_path);
	
	uint32_t self_ino = u_ino;
	char dot_name[NAME_LEN] = {0};
	dot_name[0] = '.';
	fwrite(&self_ino, sizeof(uint32_t), 1, new_fp);
	fwrite(dot_name, 1, NAME_LEN, new_fp);

	uint32_t parent_ino = current_dir;
	char dotdot_name[NAME_LEN] = {0};
	dotdot_name[0] = '.'; 
	dotdot_name[1] = '.';
	fwrite(&parent_ino, sizeof(uint32_t), 1, new_fp);
	fwrite(dotdot_name, 1, NAME_LEN, new_fp);

	fclose(new_fp);
	
	inodes_list[new_ino].active = 1;
	inodes_list[new_ino].type = 'd';
	inodes_list[new_ino].inode_num = u_ino;

}

void do_touch(char *name){
	int new_ino = -1;
	for (int i =  0; i < MAX_INODES; i++){
		if (!inodes_list[i].active){
			new_ino = 1;
			break;
		}
	}

	if(new_ino == -1) return;

	char *parent_path = uint32_to_str(current_dir);
	FILE *parent_fp = fopen(parent_path, "ab");
	free(parent_path);

	uint32_t u_ino = (uint32_t)new_ino;
	char clean_name[NAME_LEN];
	strncpy(clean_name, name, NAME_LEN);
	fwrite(&u_ino, sizeof(uint32_t), 1, parent_fp);
	fwrite(clean_name, 1, NAME_LEN, parent_fp);
	fclose(parent_fp);

	char *new_file_path = uint32_to_str(u_ino);
	FILE *file_fp = fopen(new_file_path, "w");
	fprintf(file_fp, "%s", name);
	fclose(file_fp);
	free(new_file_path);
	
	inodes_list[new_ino].active = 1;
	inodes_list[new_ino].type = 'f';
	inodes_list[new_ino].inode_num = u_ino;
}

void run_shell(){

	char line[1024];
	while (1){
		printf(">");
		if(!fgets(line, sizeof(line), stdin)) break;

		char *cmd = strtok(line, " \t\r\n\v\f");
		if (!cmd) continue;

		if(strcmp(cmd, "exit") == 0){
			save_inodes();
			break;
		} else if (strcmp(cmd, "ls") == 0){
			do_ls();
		} else if (strcmp(cmd, "cd") == 0){
			char *arg = strtok(NULL, " \t\r\n\v\f");
			if (arg) do_cd(arg);	
		} else if (strcmp(cmd, "touch") == 0){
                        char *arg = strtok(NULL, " \t\r\n\v\f");
                        if (arg) do_touch(arg);
		} else if (strcmp(cmd, "mkdir") == 0){
                        char *arg = strtok(NULL, " \t\r\n\v\f");
                        if (arg) do_mkdir(arg);
		}
	}
}

int main(int argc, char *argv[]){
	if (argc != 2){
		fprintf(stderr, "Wrong number of arguments");
		return 1;
	}

	if (chdir(argv[1]) != 0){
		perror("Error finding file system directory");
		return 1;
	}

	load_inodes();

	if (!inodes_list[0].active || inodes_list[0].type != 'd'){
		fprintf(stderr, "Error, there is no root inode");
	}

	run_shell();
	return 0;
}
