 

All Gists
BrowenChen
 
 
Star0 
Fork0
 jamesyc / gist:dd7d5362b23558326470 SECRET
Created an hour ago
 Code
 Revisions 1
Embed URL
	
HTTPS clone URL
	
You can clone with HTTPS or SSH.

 Download Gist
 gistfile1.txt Raw
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
50
51
52
53
54
55
56
57
58
59
60
61
62
63
64
65
66
67
68
69
70
71
72
73
74
75
76
77
78
79
80
81
82
83
84
85
86
87
88
89
90
91
92
93
94
95
96
97
98
99
100
101
102
103
104
105
106
107
108
109
110
111
112
113
114
115
116
117
118
119
120
121
122
123
124
125
126
127
128
129
130
131
132
133
134
135
136
137
138
139
140
141
142
143
144
145
146
147
148
149
150
151
152
153
154
155
156
157
158
159
160
161
162
163
164
165
166
167
168
169
170
171
172
173
174
175
176
177
178
179
180
181
182
183
184
185
186
187
188
189
190
191
192
193
194
195
196
197
198
199
200
201
202
203
204
205
206
207
208
209
210
211
212
213
214
215
216
217
218
219
220
221
222
223
224
225
226
227
228
229
230
231
232
233
234
235
236
237
238
239
240
241
242
243
244
245
246
247
248
249
250
251
252
253
254
255
256
257
258
259
260
261
262
263
264
265
266
267
268
269
270
271
272
273
274
275
276
277
278
279
280
281
282
283
284
285
286
287
288
289
290
291
292
293
294
295
296
297
298
299
300
301
302
303
304
305
306
307
308
309
310
311
312
313
314
315
316
317
318
319
320
321
322
323
324
325
326
327
328
329
330
331
332
333
334
335
336
337
338
339
340
341
342
343
344
345
346
347
348
349
350
351
352
353
354
355
356
357
358
359
360
361
362
363
364
365
366
367
368
369
370
371
372
373
374
375
376
377
378
379
380
381
382
383
384
385
386
387
388
389
390
391
392
393
394
395
396
397
398
399
400
401
402
403
404
405
406
407
408
409
410
411
412
413
414
415
416
417
418
419
420
421
422
423
424
425
426
427
428
429
430
431
432
433
434
435
436
437
438
439
440
441
442
443
444
445
446
447
448
449
450
451
452
453
454
455
456
457
458
459
460
461
462
463
464
465
466
467
468
469
470
471
472
473
474
475
476
477
478
479
480
481
482
483
484
485
486
487
488
489
490
491
492
493
494
495
496
497
498
499
500
501
502
503
504
505
506
507
508
509
510
511
512
513
514
515
516
517
518
519
520
521
522
523
524
525
526
527
528
529
530
531
532
533
534
535
536
537
538
539
540
541
542
543
544
545
546
547
548
549
550
551
552
553
554
555
556
557
558
559
560
561
562
563
564
565
566
567
568
569
570
571
572
573
574
575
576
577
578
579
580
581
582
583
584
585
586
587
588
589
590
591
592
593
594
595
596
597
598
599
600
601
602
603
604
605
606
607
608
609
610
611
612
613
614
615
616
617
618
619
620
621
622
623
624
625
626
627
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
 
  // 3b. Pre-Master Secret
  printf("3b. Pre-Master Secret\n");
  int plain_ps_int = random_int();
  mpz_t plain_ps;
  mpz_init(plain_ps);
  mpz_add_ui(plain_ps, plain_ps, plain_ps_int);
  mpz_t premaster_secret;
  mpz_init(premaster_secret);
  perform_rsa(premaster_secret, plain_ps, cert_exp, cert_mod);
  ps_msg client_ps;
  client_ps.type = PREMASTER_SECRET;
  mpz_get_str(client_ps.ps, SHA_BLOCK_SIZE, premaster_secret);
  err = send_tls_message(sockfd, &client_ps, PS_MSG_SIZE);
  if (err == ERR_FAILURE) {
    perror("Premaster Secret Send Error");
    cleanup();
  }
  mpz_clear(cert_exp);
  mpz_clear(cert_mod);
  mpz_clear(plain_ps);
  mpz_clear(premaster_secret);
 
  // 4. Master Secret
 
 
 
 
 
 
 
  
  
 
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
  
  // YOUR CODE HERE
  // SET AES KEYS
 
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
  mpz_t ms;
  mpz_init(ms);
  mpz_set_str(ms, ms_ver->ps, 0);
  perform_rsa(decrypted_ms, ms, key_exp, key_mod);
  mpz_clear(ms);
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
  SHA256_CTX ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, (unsigned char *) &ps, INT_SIZE);
  sha256_update(&ctx, (unsigned char *) &client_random, INT_SIZE);
  sha256_update(&ctx, (unsigned char *) &server_random, INT_SIZE);
  sha256_update(&ctx, (unsigned char *) &ps, INT_SIZE);
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
  // YOUR CODE HERE
  int err = write(socketno, msg, msg_len);
  if (err < 0) {
    return ERR_FAILURE;
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
  int err = read(socketno, msg, msg_len);
  if (err < 0) {
    return ERR_FAILURE;
  }
  // cast to hello message struct to get type
  int int_type = ((hello_message*)msg)->type;
  if (int_type != msg_type) {
    return ERR_FAILURE;
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

Write Preview Parsed as Markdown  Edit in fullscreen

Comment

Status API Blog About © 2014 GitHub, Inc. Terms Privacy Security Contact