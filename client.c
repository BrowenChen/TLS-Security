/*
 * sig_client.c
 *
 * Author: Alec Guertin
 * University of California, Berkeley
 * CS 161 - Computer Security
 * Fall 2014 Semester
 * Project 1
 */

#include "client.h"


/* The file descriptor for the socket connected to the server. */
static int sockfd;

static void perform_rsa(mpz_t result, mpz_t message, mpz_t d, mpz_t n);
static int hex_to_ascii(char a, char b);
static int hex_to_int(char a);
static void usage();
static void kill_handler(int signum);
static int random_int();
static void cleanup();

int main(int argc, char **argv) {
  


  int err, option_index, c, clientlen, counter;
  unsigned char rcv_plaintext[AES_BLOCK_SIZE];
  unsigned char rcv_ciphertext[AES_BLOCK_SIZE];
  unsigned char send_plaintext[AES_BLOCK_SIZE];
  unsigned char send_ciphertext[AES_BLOCK_SIZE];
  aes_context enc_ctx, dec_ctx;
  in_addr_t ip_addr;
  struct sockaddr_in server_addr;
  FILE *c_file, *d_file, *m_file;
  ssize_t read_size, write_size;
  struct sockaddr_in client_addr;
  tls_msg err_msg, send_msg, rcv_msg;
  mpz_t client_exp, client_mod;
  fd_set readfds;
  struct timeval tv;

  c_file = d_file = m_file = NULL;

  mpz_init(client_exp);
  mpz_init(client_mod);


  /*
   * This section is networking code that you don't need to worry about.
   * Look further down in the function for your part.
   */

  memset(&ip_addr, 0, sizeof(in_addr_t));

  option_index = 0;
  err = 0;

  static struct option long_options[] = {
    {"ip", required_argument, 0, 'i'},
    {"cert", required_argument, 0, 'c'},
    {"exponent", required_argument, 0, 'd'},
    {"modulus", required_argument, 0, 'm'},
    {0, 0, 0, 0},
  };

  while (1) {
    c = getopt_long(argc, argv, "c:i:d:m:", long_options, &option_index);
    if (c < 0) {
      break;
    }
    switch(c) {
    case 0:
      usage();
      break;
    case 'c':
      c_file = fopen(optarg, "r");
      if (c_file == NULL) {
  perror("Certificate file error");
  exit(1);
      }
      break;
    case 'd':
      d_file = fopen(optarg, "r");
      if (d_file == NULL) {
  perror("Exponent file error");
  exit(1);
      }
      break;
    case 'i':
      ip_addr = inet_addr(optarg);
      break;
    case 'm':
      m_file = fopen(optarg, "r");
      if (m_file == NULL) {
  perror("Modulus file error");
  exit(1);
      }
      break;
    case '?':
      usage();
      break;
    default:
      usage();
      break;
    }
  }

  if (d_file == NULL || c_file == NULL || m_file == NULL) {
    usage();
  }
  if (argc != 9) {
    usage();
  }

  mpz_inp_str(client_exp, d_file, 0);
  mpz_inp_str(client_mod, m_file, 0);

  signal(SIGTERM, kill_handler);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (sockfd < 0) {
    perror("Could not open socket");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = ip_addr;
  server_addr.sin_port = htons(HANDSHAKE_PORT);
  err = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
  if (err < 0) {
    perror("Could not bind socket");
    cleanup();
  }

  // YOUR CODE HERE
  // IMPLEMENT THE TLS HANDSHAKE
  // printf("Implementing TLS handshake\n");
  // printf("Client Hello\n");
  // hello_message client_h = {CLIENT_HELLO, random_int(), TLS_RSA_WITH_AES_128_ECB_SHA256};


  // err = send_tls_message(sockfd,&client_h, HELLO_MSG_SIZE);
  // if (err == ERR_FAILURE) {
  //   perror("Client Hello Error");
  //   cleanup();
  // }  

  // printf("Server Hello\n");

  // hello_message server_h = {SERVER_HELLO, random_int(), TLS_RSA_WITH_AES_128_ECB_SHA256};

  // err = receive_tls_message(sockfd, &server_h , HELLO_MSG_SIZE, SERVER_HELLO);

  // if (err == ERR_FAILURE) {
  //   perror("Client Server Error");
  //   cleanup();
  // }  



  // printf("Client Certificate\n");
  // cert_message client_certificate = {CLIENT_CERTIFICATE, c_file};

  // send_tls_message(sockfd, &client_certificate, CERT_MSG_SIZE);

  // printf("Server Certificate\n");
  // cert_message server_certificate;

  // receive_tls_message(sockfd, &server_certificate, CERT_MSG_SIZE, SERVER_CERTIFICATE);

  // printf(" ");


  // 1. Client Hello
  printf("1. Client Hello\n");
  int client_random = random_int();
  hello_message client_hello = {CLIENT_HELLO, client_random, TLS_RSA_WITH_AES_128_ECB_SHA256};
  err = send_tls_message(sockfd, &client_hello, HELLO_MSG_SIZE);
  if (err == ERR_FAILURE) {
    perror("Client Hello Error");
    cleanup();
  }
 
  // 2a. Server Hello
  printf("2a. Server Hello\n");
  hello_message server_hello;
  err = receive_tls_message(sockfd, &server_hello, HELLO_MSG_SIZE, SERVER_HELLO);
  if (err == ERR_FAILURE) {
    perror("Server Hello Error");
    cleanup();
  }
 
  // 2b. Client Certificate
  printf("2b. Client Certificate\n");
  cert_message client_cert;
  client_cert.type = CLIENT_CERTIFICATE;
  fgets(client_cert.cert, RSA_MAX_LEN, c_file);
  err = send_tls_message(sockfd, &client_cert, CERT_MSG_SIZE);
  if (err == ERR_FAILURE) {
    perror("Client Cert Error");
    cleanup();
  }
 
  // 2c. Server Certificate
  printf("2b. Server Certificate\n");
  cert_message server_cert;
  err = receive_tls_message(sockfd, &server_cert, CERT_MSG_SIZE, SERVER_CERTIFICATE);
  if (err == ERR_FAILURE) {
    perror("Server Cert Error");
    cleanup();
  }  

 // 3a. Authentication
  printf("3a. Authentication\n");
  mpz_t ca_exp;
  mpz_init(ca_exp);
  mpz_set_str(ca_exp, CA_EXPONENT, 0);
  mpz_t ca_mod;
  mpz_init(ca_mod);
  mpz_set_str(ca_mod, CA_MODULUS, 0);
  mpz_t decrypted_cert;
  mpz_init(decrypted_cert);

  decrypt_cert(decrypted_cert, &server_cert, ca_exp, ca_mod);
  mpz_clear(ca_exp);
  mpz_clear(ca_mod);

  char cert_string[RSA_MAX_LEN];
  mpz_get_ascii(cert_string, decrypted_cert);

  mpz_clear(decrypted_cert);
  printf("Server Cert string: \n");
  printf("%s\n", cert_string);
  mpz_t cert_exp;
  mpz_init(cert_exp);
  get_cert_exponent(cert_exp, cert_string);
  mpz_t cert_mod;
  mpz_init(cert_mod);
  get_cert_modulus(cert_mod, cert_string);

  printf(" ");

  // 3b. Pre-Master Secret
  printf("3b. Pre-Master Secret\n");
  int plain_ps_int = random_int();
  mpz_t plain_ps;
  mpz_init(plain_ps);
  mpz_add_ui(plain_ps, plain_ps, plain_ps_int);


  mpz_t premaster_secret;  
  mpz_init(premaster_secret);

  perform_rsa(premaster_secret, plain_ps, cert_exp, cert_mod);
  
  // printf("RSA\n");
  // unsigned char test[RSA_MAX_LEN];
  // mpz_get_ascii(test, premaster_secret);
  // printf("%s\n", test);


  ps_msg client_ps;
  client_ps.type = PREMASTER_SECRET;
  

  mpz_get_str(client_ps.ps, HEX_BASE, premaster_secret);

  printf("%s\n", client_ps.ps);

  printf("sending tls\n");
  err = send_tls_message(sockfd, &client_ps, PS_MSG_SIZE);
  if (err == ERR_FAILURE) {
    perror("Premaster Secret Send Error");
    cleanup();
  }


  printf(" ");

  // 4. Master Secret
  printf("Master Secret\n");
  ps_msg received_master_secret;

  receive_tls_message(sockfd, &received_master_secret, PS_MSG_SIZE, VERIFY_MASTER_SECRET);
  
  mpz_t decrypted_ms;
  mpz_init(decrypted_ms);

  unsigned char computed_master_secret[RSA_MAX_LEN];
  unsigned char rcv_master_secret[RSA_MAX_LEN];

  decrypt_verify_master_secret(decrypted_ms, &received_master_secret, client_exp, client_mod);

  compute_master_secret(plain_ps_int, client_hello.random, server_hello.random, computed_master_secret);

  mpz_get_ascii(rcv_master_secret, decrypted_ms);
  
  printf("%s\n", rcv_master_secret);
  int different = 0;
  int x; 

  printf("testing\n");

  for (x = 0; x < RSA_MAX_LEN; x++){
    if (computed_master_secret[x] != rcv_master_secret[x]){
      
      different += different;
    }
  }

  printf("%d asdasdas", different);
  printf(" ");
  fflush(stdout);
  if (different > 0){
    printf("Computed and given master secrets aren't correct");

    cleanup();

  }
  // mpz_get_str(rcv_master_secret, HEX_BASE, (uchar)received_master_secret.ps);
  // rcv_master_secret = received_master_secret.ps;
  // unsigned char rcv_master_secret[16] = received_master_secret.ps;
  



  mpz_clear(cert_exp);
  mpz_clear(cert_mod);
  mpz_clear(plain_ps);
  mpz_clear(premaster_secret);
  mpz_clear(decrypted_ms);


 
 
  
  /*
   * START ENCRYPTED MESSAGES
   */

  memset(send_plaintext, 0, AES_BLOCK_SIZE);
  memset(send_ciphertext, 0, AES_BLOCK_SIZE);
  memset(rcv_plaintext, 0, AES_BLOCK_SIZE);
  memset(rcv_ciphertext, 0, AES_BLOCK_SIZE);

  memset(&rcv_msg, 0, TLS_MSG_SIZE);

  aes_init(&enc_ctx);
  aes_init(&dec_ctx);
  
  // YOUR CODE HEREenc_ctx, dec_ctx;
  // SET AES KEYS

  aes_setkey_enc( &enc_ctx, rcv_master_secret,
                    128 );

  aes_setkey_dec( &dec_ctx, rcv_master_secret,
                    128 );



  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
  /* Send and receive data. */
  while (1) {
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sockfd, &readfds);
    tv.tv_sec = 2;
    tv.tv_usec = 10;

    select(sockfd+1, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
      counter = 0;
      memset(&send_msg, 0, TLS_MSG_SIZE);
      send_msg.type = ENCRYPTED_MESSAGE;
      memset(send_plaintext, 0, AES_BLOCK_SIZE);
      read_size = read(STDIN_FILENO, send_plaintext, AES_BLOCK_SIZE);
      while (read_size > 0 && counter + AES_BLOCK_SIZE < TLS_MSG_SIZE - INT_SIZE) {
  if (read_size > 0) {
    err = aes_crypt_ecb(&enc_ctx, AES_ENCRYPT, send_plaintext, send_ciphertext);
    memcpy(send_msg.msg + counter, send_ciphertext, AES_BLOCK_SIZE);
    counter += AES_BLOCK_SIZE;
  }
  memset(send_plaintext, 0, AES_BLOCK_SIZE);
  read_size = read(STDIN_FILENO, send_plaintext, AES_BLOCK_SIZE);
      }
      write_size = write(sockfd, &send_msg, INT_SIZE+counter+AES_BLOCK_SIZE);
      if (write_size < 0) {
  perror("Could not write to socket");
  cleanup();
      }
    } else if (FD_ISSET(sockfd, &readfds)) {
      memset(&rcv_msg, 0, TLS_MSG_SIZE);
      memset(rcv_ciphertext, 0, AES_BLOCK_SIZE);
      read_size = read(sockfd, &rcv_msg, TLS_MSG_SIZE);
      if (read_size > 0) {
  if (rcv_msg.type != ENCRYPTED_MESSAGE) {
    goto out;
  }
  memcpy(rcv_ciphertext, rcv_msg.msg, AES_BLOCK_SIZE);
  counter = 0;
  while (counter < read_size - INT_SIZE - AES_BLOCK_SIZE) {
    aes_crypt_ecb(&dec_ctx, AES_DECRYPT, rcv_ciphertext, rcv_plaintext);
    printf("%s", rcv_plaintext);
    counter += AES_BLOCK_SIZE;
    memcpy(rcv_ciphertext, rcv_msg.msg+counter, AES_BLOCK_SIZE);
  }
  printf("\n");
      }
    }

  }

 out:
  close(sockfd);
  return 0;
}

/*
 * \brief                  Decrypts the certificate in the message cert.
 *
 * \param decrypted_cert   This mpz_t stores the final value of the binary
 *                         for the decrypted certificate. Write the end
 *                         result here.
 * \param cert             The message containing the encrypted certificate.
 * \param key_exp          The exponent of the public key for decrypting
 *                         the certificate.
 * \param key_mod          The modulus of the public key for decrypting
 *                         the certificate.
 */
void
decrypt_cert(mpz_t decrypted_cert, cert_message *cert, mpz_t key_exp, mpz_t key_mod)
{
  // YOUR CODE HERE
  printf("decrypt cert \n");
  printf("%d\n", cert->type);
  
  mpz_t message;
  mpz_init(message);
  mpz_set_str(message, cert->cert, 0);
  perform_rsa(decrypted_cert, message, key_exp, key_mod);

  mpz_clear(message); 
}

/*
 * \brief                  Decrypts the master secret in the message ms_ver.
 *
 * \param decrypted_ms     This mpz_t stores the final value of the binary
 *                         for the decrypted master secret. Write the end
 *                         result here.
 * \param ms_ver           The message containing the encrypted master secret.
 * \param key_exp          The exponent of the public key for decrypting
 *                         the master secret.
 * \param key_mod          The modulus of the public key for decrypting
 *                         the master secret.
 */
void
decrypt_verify_master_secret(mpz_t decrypted_ms, ps_msg *ms_ver, mpz_t key_exp, mpz_t key_mod)
{
  // YOUR CODE HERE
  // Decrypt ms_ver with you private key/

  mpz_t z; 
  mpz_init(z);

  // int i=0,j=0;
  // char c, client_private[100];
  // //Get client private key through command line
  // while ( (c=getchar()) != '\n')
  // {
  //   client_private[i++]=c;
  // }

  //Decrypt with client's private key
  mpz_set_str(z, ms_ver->ps, HEX_BASE);
  perform_rsa(decrypted_ms, z, key_exp, key_mod);
  mpz_clear(z);


}

/*
 * \brief                  Computes the master secret.
 *
 * \param ps               The premaster secret.
 * \param client_random    The random value from the client hello.
 * \param server_random    The random value from the server hello.
 * \param master_secret    A pointer to the final value of the master secret.
 *                         Write the end result here.
 */
void
compute_master_secret(int ps, int client_random, int server_random, unsigned char *master_secret)
{
  // YOUR CODE HERE
  // Master secret = H(P S||clienthello.random||serverhello.random||P S)

// Create an SHA256_CTX object.
// Initialize it with sha256_init().
// Read some/all of the data to hash into an array, calculate the size of the data, and add it to the hash with sha256_update().
// Repeat the previous step for all the data you want to hash.
// Finalize and output the hash with sha256_final().

  SHA256_CTX ctx;
  sha256_init(&ctx);
  int intData[4] = {ps, client_random, server_random, ps};
  unsigned char *data = (unsigned char *)intData;

  sha256_update(&ctx, data, 16);  
  // sha256_update(&ctx, (uchar*) ps ,INT_SIZE);
  // sha256_update(&ctx, (uchar*) client_random ,INT_SIZE);
  // sha256_update(&ctx, (uchar*) server_random ,INT_SIZE);
  // sha256_update(&ctx, (uchar*) ps ,INT_SIZE);
  sha256_final(&ctx, master_secret);



}

/*
 * \brief                  Sends a message to the connected server.
 *                         Returns an error code.
 *
 * \param socketno         A file descriptor for the socket to send
 *                         the message on.
 * \param msg              A pointer to the message to send.
 * \param msg_len          The length of the message in bytes.
 */
int
send_tls_message(int socketno, void *msg, int msg_len)
{
  
  fflush(stdout);
  // YOUR CODE HERE
  // Take in msg and send to socketno

  int errCode = write(socketno, (const void *) msg, msg_len);
  //printf( "testing\n");
  
  //printf("%d\n", socketno);
  //printf("%d\n", msg_len);
  
  fflush(stdout);

  printf("errCode: %d\n", errCode);

  if ( errCode < 0){
    return ERR_FAILURE;
    cleanup();
  }
  return ERR_OK;


}

/*
 * \brief                  Receieves a message from the connected server.
 *                         Returns an error code.
 *
 * \param socketno         A file descriptor for the socket to receive
 *                         the message on.
 * \param msg              A pointer to where to store the received message.
 * \param msg_len          The length of the message in bytes.
 * \param msg_type         The expected type of the message to receive.
 */
int
receive_tls_message(int socketno, void *msg, int msg_len, int msg_type)
{
  // YOUR CODE HERE

  // Get the defined length of the msg_type in handsake and check if matches msg_len

  // Check hello_message, cert_message, ps_msg, tsl_msg types
  printf("Receiving\n");

  fflush(stdout);

  // printf("%d", (struct hello_message) msg->type);
  int err = read(socketno, msg, msg_len);


  printf("err: %d\n", err);

  if (err < 0){
    return ERR_FAILURE;
    cleanup();
  }


  // cast to hello message struct to get type
  int int_type = ((hello_message*)msg)->type;
  if (int_type != msg_type) {
    if (int_type == ERROR_MESSAGE){
      printf("%s\n", ((tls_msg*)msg)->msg);
    }
    return ERR_FAILURE;
    cleanup();
  }
  return ERR_OK;


}


/*
 * \brief                Encrypts/decrypts a message using the RSA algorithm.
 *
 * \param result         a field to populate with the result of your RSA calculation.
 * \param message        the message to perform RSA on. (probably a cert in this case)
 * \param e              the encryption key from the key_file passed in through the
 *                       command-line arguments
 * \param n              the modulus for RSA from the modulus_file passed in through
 *                       the command-line arguments
 *
 * Fill in this function with your proj0 solution or see staff solutions.
 */
static void
perform_rsa(mpz_t result, mpz_t message, mpz_t e, mpz_t n)
{

    /* YOUR CODE HERE */
  int odd_num;

  mpz_set_str(result, "1", 10);
  odd_num = mpz_odd_p(e);
  while (mpz_cmp_ui(e, 0) > 0) {
    if (odd_num) {
      mpz_mul(result, result, message);
      mpz_mod(result, result, n);
      mpz_sub_ui(e, e, 1);
    }
    mpz_mul(message, message, message);
    mpz_mod(message, message, n);
    mpz_div_ui(e, e, 2);
    odd_num = mpz_odd_p(e);
  }


  // printf("Testing ");

}


/* Returns a pseudo-random integer. */
static int
random_int()
{
  srand(time(NULL));
  return rand();
}

/*
 * \brief                 Returns ascii string from a number in mpz_t form.
 *
 * \param output_str      A pointer to the output string.
 * \param input           The number to convert to ascii.
 */
void
mpz_get_ascii(char *output_str, mpz_t input)
{
  int i,j;
  char *result_str;
  result_str = mpz_get_str(NULL, HEX_BASE, input);
  i = 0;
  j = 0;
  while (result_str[i] != '\0') {
    output_str[j] = hex_to_ascii(result_str[i], result_str[i+1]);
    j += 1;
    i += 2;
  }
}

/*
 * \brief                  Returns a pointer to a string containing the
 *                         characters representing the input hex value.
 *
 * \param data             The input hex value.
 * \param data_len         The length of the data in bytes.
 */
char
*hex_to_str(char *data, int data_len)
{
  int i;
  char *output_str = calloc(1+2*data_len, sizeof(char));
  for (i = 0; i < data_len; i += 1) {
    snprintf(output_str+2*i, 3, "%02X", (unsigned int) (data[i] & 0xFF));
  }
  return output_str;
}

/* Return the public key exponent given the decrypted certificate as string. */
int
get_cert_exponent(mpz_t result, char *cert)
{
  int err;
  char *srch, *srch2;
  char exponent[RSA_MAX_LEN/2];
  memset(exponent, 0, RSA_MAX_LEN/2);
  srch = strchr(cert, '\n');
  if (srch == NULL) {
    return ERR_FAILURE;
  }
  srch += 1;
  srch = strchr(srch, '\n');
  if (srch == NULL) {
    return ERR_FAILURE;
  }
  srch += 1;
  srch = strchr(srch, '\n');
  if (srch == NULL) {
    return ERR_FAILURE;
  }
  srch += 1;
  srch = strchr(srch, ':');
  if (srch == NULL) {
    return ERR_FAILURE;
  }
  srch += 2;
  srch2 = strchr(srch, '\n');
  if (srch2 == NULL) {
    return ERR_FAILURE;
  }
  strncpy(exponent, srch, srch2-srch);
  err = mpz_set_str(result, exponent, 0);
  if (err == -1) {
    return ERR_FAILURE;
  }
  return ERR_OK;
}

/* Return the public key modulus given the decrypted certificate as string. */
int
get_cert_modulus(mpz_t result, char *cert)
{
  int err;
  char *srch, *srch2;
  char modulus[RSA_MAX_LEN/2];
  memset(modulus, 0, RSA_MAX_LEN/2);
  srch = strchr(cert, '\n');
  if (srch == NULL) {
    return ERR_FAILURE;
  }
  srch += 1;
  srch = strchr(srch, '\n');
  if (srch == NULL) {
    return ERR_FAILURE;
  }
  srch += 1;
  srch = strchr(srch, ':');
  if (srch == NULL) {
    return ERR_FAILURE;
  }
  srch += 2;
  srch2 = strchr(srch, '\n');
  if (srch2 == NULL) {
    return ERR_FAILURE;
  }
  strncpy(modulus, srch, srch2-srch);
  err = mpz_set_str(result, modulus, 0);
  if (err == -1) {
    return ERR_FAILURE;
  }
  return ERR_OK;
}

/* Prints the usage string for this program and exits. */
static void
usage()
{
    printf("./client -i <server_ip_address> -c <certificate_file> -m <modulus_file> -d <exponent_file>\n");
    printf("Testing");
    printf("%d\n", random_int());
    
    
    printf("%d\n", random_int());
    printf("%d\n", random_int());

    exit(1);
}

/* Catches the signal from C-c and closes connection with server. */
static void
kill_handler(int signum)
{
  if (signum == SIGTERM) {
    cleanup();
  }
}

/* Converts the two input hex characters into an ascii char. */
static int
hex_to_ascii(char a, char b)
{
    int high = hex_to_int(a) * 16;
    int low = hex_to_int(b);
    return high + low;
}

/* Converts a hex value into an int. */
static int
hex_to_int(char a)
{
    if (a >= 97) {
  a -= 32;
    }
    int first = a / 16 - 3;
    int second = a % 16;
    int result = first*10 + second;
    if (result > 9) {
  result -= 1;
    }
    return result;
}

/* Closes files and exits the program. */
static void
cleanup()
{
  close(sockfd);
  exit(1);
}
