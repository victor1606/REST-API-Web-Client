#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>      /* printf, sprintf */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <ctype.h>
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define CMD_LEN 100
#define CREDENTIALS_LEN 50
#define BOOK_PARAM_LEN 80
#define COOKIE_LEN 100
#define ID_LEN 20
#define BOOK_PATH_LEN 100
#define HOST_ADDR "34.241.4.235"
#define REGISTER_PATH "/api/v1/tema/auth/register"
#define LOGIN_PATH "/api/v1/tema/auth/login"
#define ENTER_PATH "/api/v1/tema/library/access"
#define LIB_PATH "/api/v1/tema/library/books"
#define LOGOUT_PATH "/api/v1/tema/auth/logout"
#define JSON_APP_TYPE "application/json"
#define PORT 8080

int main(int argc, char *argv[]) {
	char *message;
	char *response;

	char *cookie = calloc(COOKIE_LEN, sizeof(char));
	if (!cookie) {
		error("Malloc failed!\n");
	}

	char *jwt = NULL;

	int logged_in_flag = 0;

	while (1) {
		char cmd[CMD_LEN];
		scanf("%s", cmd);

		if (!strcmp(cmd, "login")) {
			int sockfd = open_connection(HOST_ADDR, PORT, AF_INET, SOCK_STREAM, 0);
			char username[CREDENTIALS_LEN];
			char password[CREDENTIALS_LEN];
			printf("username=");
			scanf("%s", username);
			printf("password=");
			scanf("%s", password);

			if (logged_in_flag == 1) { // keeps track of login session
				printf("Already logged in!\n");
				continue;
			}

			// using Parson to form JSON body
			JSON_Value *root_value = json_value_init_object();
			JSON_Object *root_object = json_value_get_object(root_value);

			json_object_set_string(root_object, "username", username);
			json_object_set_string(root_object, "password", password);
			char *body_data = json_serialize_to_string_pretty(root_value);

			message = compute_post_request(HOST_ADDR, LOGIN_PATH, JSON_APP_TYPE, NULL, &body_data, 1, NULL, 0);
				
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			// checking for error codes & printing results
			if (strstr(response, "200 OK")) {
				printf("200 - OK - Logged in with username: %s.\n", username);

				// searches response for session cookie
				char *aux_cookie = strstr(response, "Set-Cookie: connect.sid");
				aux_cookie += strlen("Set-Cookie: ");

				if (cookie == NULL) { // checking if a session cookie has already been malloc'd
					cookie = malloc(COOKIE_LEN * sizeof(char));
				}

				sscanf(aux_cookie, "%s ", cookie); // extracts cookie
				cookie[strlen(cookie) - 1] = '\0';

				logged_in_flag = 1; // marks login session
			} else {
				printf("Invalid credentials!\n");
			}

			json_free_serialized_string(body_data);
			json_value_free(root_value);

			close_connection(sockfd);
			continue;
		} else if (!strcmp(cmd, "register")) {
			int sockfd = open_connection(HOST_ADDR, PORT, AF_INET, SOCK_STREAM, 0);
			char username[CREDENTIALS_LEN];
			char password[CREDENTIALS_LEN];
			printf("username=");
			scanf("%s", username);
			printf("password=");
			scanf("%s", password);

			JSON_Value *root_value = json_value_init_object();
			JSON_Object *root_object = json_value_get_object(root_value);

			json_object_set_string(root_object, "username", username);
			json_object_set_string(root_object, "password", password);
			char *body_data = json_serialize_to_string_pretty(root_value);

			message = compute_post_request(HOST_ADDR, REGISTER_PATH, JSON_APP_TYPE, NULL, &body_data, 1, NULL, 0);

			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			if (strstr(response, "Created")) {
				printf("201 - OK - Registered new account.\n");
			} else if (strstr(response, "is taken!")){
				printf("400 - Bad Request - An account with the given username already exists!\n");
			} else {
				printf("Error. Try again later!\n");
			}

			json_free_serialized_string(body_data);
    		json_value_free(root_value);

			close_connection(sockfd);
		} else if (!strcmp(cmd, "enter_library")) {
			int sockfd = open_connection(HOST_ADDR, PORT, AF_INET, SOCK_STREAM, 0);
			int cookie_size = 1;
			char **cookie_value = &cookie;

			// checking if a session cookie has already been set
			// if it hasn't, send NULL for value and 0 for count
			if (cookie == NULL) {
				cookie_value = NULL;
				cookie_size = 0;
			}

			message = compute_get_request(HOST_ADDR, ENTER_PATH, NULL, NULL, cookie_value, cookie_size);

			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			if (strstr(response, "200 OK")) {
				printf("200 - OK - Entered library.\n");

				// searching for & extracting JWT authorization
				jwt = strstr(response, "token\":");
				jwt += strlen("token\":\"");
				jwt[strlen(jwt) - 2] = '\0';
			} else {
				printf("401 - Not logged in / Session cookie invalid.\n");
			}

			close_connection(sockfd);
		} else if (!strcmp(cmd, "get_books")) {
			if (!jwt || logged_in_flag != 1) {
				printf("No JWT found! Login required/Enter library.\n");
				continue;
			}

			int sockfd = open_connection(HOST_ADDR, PORT, AF_INET, SOCK_STREAM, 0);

			message = compute_get_request(HOST_ADDR, LIB_PATH, NULL, jwt, NULL, 0);

			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			if (strstr(response, "200 OK")) {
				printf("200 - OK - Got books:\n");

				// extracting books list from response
				char *books = strstr(response, "[{");
				if (books != NULL) {
					printf("%s\n", books);
				} else {
					printf("Library is empty!\n");
				}
			} else if (strstr(response, "403 Forbidden")) {
				printf("403 - Forbidden - You do not have access to the library!.\n");
			}

			close_connection(sockfd);
		} else if (!strcmp(cmd, "get_book")) {
			printf("id=");
			char id[ID_LEN];
			scanf("%s", id);

			if (jwt == NULL || logged_in_flag != 1 || strlen(jwt) == 0) {
				printf("No JWT found! Login && Library access required.\n");
				continue;
			}

			int sockfd = open_connection(HOST_ADDR, PORT, AF_INET, SOCK_STREAM, 0);

			char book_path[BOOK_PATH_LEN] = "/api/v1/tema/library/books/";
			strcat(book_path, id);

			message = compute_get_request(HOST_ADDR, book_path, NULL, jwt, NULL, 0);

			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			if (strstr(response, "200 OK")) {
				printf("200 - OK - Got book:\n");

				char *book = strstr(response, "[{\"");
				if (book != NULL) {
					printf("%s\n", book);
				}

			} else if (strstr(response, "id is not")) {
				printf("400 - Bad Request - Invalid id.\n");
			} else if (strstr(response, "No book was found!")) {
				printf("404 - Not Found - No book found with the given id.\n");
			}

			close_connection(sockfd);
		} else if (!strcmp(cmd, "add_book")) {
			if (!jwt) {
				printf("No JWT found! Login required.\n");
				continue;
			}

			int sockfd = open_connection(HOST_ADDR, PORT, AF_INET, SOCK_STREAM, 0);
			
			// removes the \n character added by the previous scanf
			getchar();

			// all book parameters (besides page_count) can contain spaces or numbers
			printf("title=");
            char title[BOOK_PARAM_LEN];
			fgets(title, BOOK_PARAM_LEN, stdin);
			title[strlen(title) - 1] = '\0';

            printf("author=");
            char author[BOOK_PARAM_LEN];
			fgets(author, BOOK_PARAM_LEN, stdin);
			author[strlen(author) - 1] = '\0';

            printf("genre=");
            char genre[BOOK_PARAM_LEN];
			fgets(genre, BOOK_PARAM_LEN, stdin);
			genre[strlen(genre) - 1] = '\0';

            printf("publisher=");
            char publisher[BOOK_PARAM_LEN];
			fgets(publisher, BOOK_PARAM_LEN, stdin);
			publisher[strlen(publisher) - 1] = '\0';

            printf("page_count=");
            char pg_ct[BOOK_PARAM_LEN];
			fgets(pg_ct, BOOK_PARAM_LEN, stdin);
			pg_ct[strlen(pg_ct) - 1] = '\0';
			
			// if pg_ct contains letters / is negative, throw an error
			int letter_flag = 0;
			for (int i = 0; pg_ct[i] != '\0'; ++i) {
				if (isdigit(pg_ct[i]) && pg_ct[i] >= 0) {
					continue;
				} else {
					letter_flag++;
					break;
				}
			}

			if (letter_flag != 0) {
				printf("Invalid page_count!\n");
				close_connection(sockfd);
				continue;
			}

			JSON_Value *root_value = json_value_init_object();
			JSON_Object *root_object = json_value_get_object(root_value);

			json_object_set_string(root_object, "title", title);
			json_object_set_string(root_object, "author", author);
			json_object_set_string(root_object, "genre", genre);
			json_object_set_number(root_object, "page_count", atoi(pg_ct));
			json_object_set_string(root_object, "publisher", publisher);
			char *body_data = json_serialize_to_string_pretty(root_value);

			message = compute_post_request(HOST_ADDR, LIB_PATH, JSON_APP_TYPE, jwt, &body_data, 1, NULL, 0);
			
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			if (strstr(response, "200 OK")) {
				printf("200 - OK - Added book succesfully.\n");
			} else if (strstr(response, "429 Too Many Requests")) {
				printf("Too Many Requests - Try again later.\n");
			} else if (strstr(response, "500 Internal Server Error")) {
				printf("500 - Internal Server Error - Bad format/Bad token.\n");
			}

			close_connection(sockfd);
		} else if (!strcmp(cmd, "delete_book")) {
			if (!jwt) {
				printf("No JWT found! Login required.\n");
				continue;
			}
			
			int sockfd = open_connection(HOST_ADDR, PORT, AF_INET, SOCK_STREAM, 0);

			printf("id=");
			char id[ID_LEN];
			scanf("%s", id);

			// concatenate id to path
			char book_path[BOOK_PATH_LEN] = "/api/v1/tema/library/books/";
			strcat(book_path, id);

	    	message = compute_delete_request(HOST_ADDR, book_path, jwt);
			
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			if (strstr(response, "200 OK")) {
				printf("200 - OK - Deleted book.\n");
			} else if (strstr(response, "403 Forbidden")) {
				printf("403 - Forbidden - No JWT found!\n");
			} else if (strstr(response, "404 Not Found")) {
				printf("404 - Not Found - Book not found - Invalid ID!\n");
			} else if (strstr(response, "500 Internal Server Error")) {
				printf("500 Internal Server Error - Invalid ID!\n");
			}
			close_connection(sockfd);			
		} else if (!strcmp(cmd, "logout")) {
			if (logged_in_flag != 1) {
				printf("Not logged in!\n");
				continue;
			}
			int sockfd = open_connection(HOST_ADDR, PORT, AF_INET, SOCK_STREAM, 0);
			
			int cookie_size = 1;
			char **cookie_value = &cookie;

			if (cookie == NULL) {
				cookie_value = NULL;
				cookie_size = 0;
			}

			message = compute_get_request(HOST_ADDR, LOGOUT_PATH, NULL, jwt, cookie_value, cookie_size);

			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			if (strstr(response, "200 OK")) {
				printf("200 - OK - Logged out succesfully.\n");
			} else if (strstr(response, "You are not logged in!")) {
				printf("400 - You are not logged in!\n");
			}

			// free session cookie and jwt if needed
			if (cookie != NULL) {
				free(cookie);
				cookie = NULL;
			}
			if (jwt != NULL) {
				jwt = NULL;
			}

			logged_in_flag = 0;

			close_connection(sockfd);
		} else if (!strcmp(cmd, "exit")) {
			// free session cookie and jwt if needed
			if (cookie != NULL) {
				free(cookie);
				cookie = NULL;
			}
			if (jwt != NULL) {
				jwt = NULL;
			}
			
			return 0;
		} else {
			printf("Unrecognized command.\n");
		}
	}

	return 0;
}
