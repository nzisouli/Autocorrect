//Autocorrect progam that takes as input a dictionary and a text and corrects
//the spelling errors in the texts according to the dictionary

#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#define BUFF_SIZE 20
#define END_OF_FILE -2
#define NOT_READING -1
#define NO_ERROR 0
#define LSEEK_PROB -3

struct word {
	char buff[BUFF_SIZE];
	int error;
	int space_punct_no; //number of non alaphanumeric characters after the read word
};

//function that finds a word from a file
struct word find (int fd, int length, char *byte) {
	int i=0;
	off_t current_fd;
	struct word buffer;
	
	buffer.space_punct_no = 0;
	buffer.error = NO_ERROR;
	
	//read per word byte until finding a non alphanumeric character
	do {
		if (read(fd, byte, 1) != 1) {
			perror("Problem reading 1");
			strcpy(buffer.buff, "");
			buffer.error = NOT_READING;
			return buffer;
		}
		if (isalpha(*byte)!=0) {
			buffer.buff[i] = *byte;
			i++;
		}
	}
	while (isalpha(*byte)!=0);
	
	//complete buffer with '\0'
	buffer.buff[i] = '\0';
	
	//check if there is a next word or the file ends
	while (isalpha(*byte)==0) {
		buffer.space_punct_no ++;
		current_fd = lseek(fd, 0, SEEK_CUR);
		if (current_fd < 0) {
			perror("Problem lseek 6");
			strcpy(buffer.buff, "");
			buffer.error = LSEEK_PROB;
			return buffer;
		}
		if(current_fd == length+1) {
			buffer.error = END_OF_FILE;
			return buffer;
		}
		if (*byte == '\n') {
			return buffer;
		}
		if (read(fd, byte, 1) !=1) {
			perror("Problem reading 2");
			strcpy(buffer.buff, "");
			buffer.error = NOT_READING;
			return buffer;
		}
	}
	
	//the first letter of the next word is read
	if (lseek(fd, -1, SEEK_CUR) < 0) {
		perror("Problem lseek 7");
		strcpy(buffer.buff, "");
		buffer.error = LSEEK_PROB;
	}
	
	return buffer;
}

int main (int argc, char *argv[]) {
	struct word text_buffer, dict_buffer, right_word;
	int dictionary_fd, text_fd, dict_len, text_len, i, length_difference;
	off_t new_word_start, word_end, offset;  //offset: saves the beggining of the next line of dictionary
	char temp[4], byte='\0', temp_byte;

	//open dictionary and text files
	dictionary_fd = open(argv[1], O_RDWR, S_IRWXU);
	if (dictionary_fd < 0) {
		perror("Opening dictionary failed");
		return 1;
	}
	dict_len = lseek(dictionary_fd, -1, SEEK_END);
	if (dict_len < 0) {
		perror("Problem lseek 1");
		close (dictionary_fd);
		return 1;
	}
	if (lseek(dictionary_fd, 0, SEEK_SET) <0) {
		perror("Problem lseek 2");
		close (dictionary_fd);
		return 1;
	}
	
	text_fd = open(argv[2], O_RDWR, S_IRWXU);
	if (text_fd < 0) {
		perror("Opening text failed");
		close (dictionary_fd);
		return 1;
	}
	text_len = lseek(text_fd, -1, SEEK_END);
	if (text_len < 0) {
		perror("Problem lseek 3");
		close (dictionary_fd);
		close (text_fd);
		return 1;
	}
	if (lseek(text_fd, 0, SEEK_SET) < 0) {
		perror("Problem lseek 4");
		close (dictionary_fd);
		close (text_fd);
		return 1;
	}
	
	text_buffer.error = NO_ERROR;
	
	while (text_buffer.error != END_OF_FILE) {
		text_buffer.error = NO_ERROR;
		
		//read one by one the words of the text
		text_buffer = find (text_fd, text_len, &byte);
		if (text_buffer.error == NOT_READING || text_buffer.error == LSEEK_PROB) {
			close (dictionary_fd);
			close (text_fd);
			return 1;
		}
		
		printf("Text word: %s\n", text_buffer.buff);
		
		byte = '\0';
		
		offset = lseek (dictionary_fd, 0, SEEK_SET);
		if (offset < 0) {
			perror("Problem lseek 8");
			close (dictionary_fd);
			close (text_fd);
			return 1;
		}
		
		//read one by one the words of dictionary until finding the word of text
		do {
			dict_buffer.error = NO_ERROR;
			
			if (byte == '\n') {
				offset = lseek (dictionary_fd, 0, SEEK_CUR);
				if (offset < 0) {
					perror("Problem lseek 9");
					close (dictionary_fd);
					close (text_fd);
					return 1;
				}
			}
			
			dict_buffer = find (dictionary_fd, dict_len, &byte);
			
			if (dict_buffer.error == NOT_READING || dict_buffer.error == LSEEK_PROB) {
				close (dictionary_fd);
				close (text_fd);
				return 1;
			}
			
			printf("Dict word: %s\n", dict_buffer.buff);
		}
		while (strcmp(dict_buffer.buff, text_buffer.buff) != 0 && dict_buffer.error != END_OF_FILE);
		
		//if a text word is found in dictionary check if it is wright or wrong
		if (strcmp(dict_buffer.buff, text_buffer.buff) == 0) {
			if (lseek (dictionary_fd, -3, SEEK_CUR) < 0) {
				perror("Problem lseek 10");
				close (dictionary_fd);
				close (text_fd);
				return 1;
			}
			if (read (dictionary_fd, temp, 3) != 3) {
				perror("Problem reading3");
				close (dictionary_fd);
				close (text_fd);
				return 1;
			}
			
			//in case thw text word is wrong
			if (strcmp (temp, " - ") != 0) {
				if (lseek (dictionary_fd, offset, SEEK_SET) < 0) {
					perror("Problem lseek 11");
					close (dictionary_fd);
					close (text_fd);
					return 1;
				}
				
				//find thw right word and repalce
				right_word = find (dictionary_fd, dict_len, &byte);
				
				if (right_word.error == NOT_READING || right_word.error == LSEEK_PROB) {
					close (dictionary_fd);
					close (text_fd);
					return 1;
				}
				
				printf("Right word: %s\n", right_word.buff);
				
				length_difference = strlen (right_word.buff) - strlen (text_buffer.buff);
				
				//in case the right word has the same length as the wrong one
				if (length_difference == 0) {
					//text_fd goes to the beginning of the word to be replaced
					if (lseek (text_fd, (off_t) (-(strlen(text_buffer.buff) + text_buffer.space_punct_no)), SEEK_CUR) < 0) {
						perror("Problem lseek 12");
						close (dictionary_fd);
						close (text_fd);
						return 1;
					}
					
					if (write (text_fd, right_word.buff, strlen(right_word.buff)) != strlen(right_word.buff)) {
						perror("problem writing1");
						close (dictionary_fd);
						close (text_fd);
						return 1;
					}
					
					if (lseek (text_fd, text_buffer.space_punct_no, SEEK_CUR) < 0) {
						perror("Problem lseek 13");
						close (dictionary_fd);
						close (text_fd);
						return 1;
					}
				}
				else {
					//in case the right word has smaller length than the wrong one
					if (length_difference < 0) {
						//text_fd goes to the end of the word to be replaced
						word_end = lseek (text_fd, (off_t) (-text_buffer.space_punct_no), SEEK_CUR);
						if (word_end < 0) {
							perror("Problem lseek 14");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						for (i=word_end ; i<=text_len ; i++) {
							if (lseek (text_fd, (off_t) i, SEEK_SET) < 0) {
								perror("Problem lseek 15");
								close (dictionary_fd);
								close (text_fd);
								return 1;
							}
							if (read (text_fd, &temp_byte, 1) != 1) {
								perror("Problem reading 3");
								close (dictionary_fd);
								close (text_fd);
								return 1;
							}
							if (lseek (text_fd, (off_t) (length_difference - 1), SEEK_CUR) < 0) {
								perror("Problem lseek 16");
								close (dictionary_fd);
								close (text_fd);
								return 1;
							}
							if (write (text_fd, &temp_byte, 1) != 1) {
								perror("problem writing 2");
								close (dictionary_fd);
								close (text_fd);
								return 1;
							}
						}
						
						//text_fd goes to the beginning of the word to be replaced
						if (lseek (text_fd, (off_t) (word_end - strlen(text_buffer.buff)), SEEK_SET) < 0) {
							perror("Problem lseek 17");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						if (write (text_fd, right_word.buff, strlen(right_word.buff)) != strlen(right_word.buff)) {
							perror("problem writing 3");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						new_word_start = lseek (text_fd, text_buffer.space_punct_no, SEEK_CUR);
						if (new_word_start < 0) {
							perror("Problem lseek 18");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						ftruncate (text_fd, text_len + length_difference + 1);
						
						text_len = lseek(text_fd, -1, SEEK_END);
						if (text_len < 0) {
							perror("Problem lseek 19");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						if (lseek (text_fd, new_word_start, SEEK_SET) < 0) {
							perror("Problem lseek 20");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
					}
					//in case the right word haw bigger length than the wrong one
					else {
						//text_fd goes to the end of the word to be replaced
						word_end = lseek (text_fd, (off_t) (-text_buffer.space_punct_no), SEEK_CUR);
						if (word_end < 0) {
							perror("Problem lseek 21");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						for (i=text_len ; i>=word_end ; i--) {
							if (lseek (text_fd, (off_t) i, SEEK_SET) < 0) {
								perror("Problem lseek 22");
								close (dictionary_fd);
								close (text_fd);
								return 1;
							}
							if (read (text_fd, &temp_byte, 1) != 1) {
								perror("Problem reading 4");
								close (dictionary_fd);
								close (text_fd);
								return 1;
							}
							if (lseek (text_fd, (off_t) (length_difference - 1), SEEK_CUR) < 0) {
								perror("Problem lseek 23");
								close (dictionary_fd);
								close (text_fd);
								return 1;
							}
							if (write (text_fd, &temp_byte, 1) != 1) {
								perror("problem writing 4");
								close (dictionary_fd);
								close (text_fd);
								return 1;
							}
						}
						
						//text_fd goes to the beginning of the word to be replaced
						if (lseek (text_fd, (off_t) (word_end - strlen(text_buffer.buff)), SEEK_SET) < 0) {
							perror("Problem lseek 24");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						if (write (text_fd, right_word.buff, strlen(right_word.buff)) != strlen(right_word.buff)) {
							perror("problem writing 5");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						new_word_start = lseek (text_fd, text_buffer.space_punct_no, SEEK_CUR);
						if (new_word_start < 0) {
							perror("Problem lseek 25");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						text_len = lseek(text_fd, -1, SEEK_END);
						if (text_len < 0) {
							perror("Problem lseek 26");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
						
						if (lseek (text_fd, new_word_start, SEEK_SET) < 0) {
							perror("Problem lseek 27");
							close (dictionary_fd);
							close (text_fd);
							return 1;
						}
					}
				}
			}
		}
	}
	
	close (dictionary_fd);
	close (text_fd);
	
	return 0;
}