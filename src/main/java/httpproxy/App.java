package httpproxy;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Hello world!
 */
public class App {
    private final static Logger logger = Logger.getLogger(App.class.getName());

    public static void main(String[] args) {
        // Java Server socket bind to port 8989
        try (ServerSocket serverSocket = new ServerSocket(8989)) {
            logger.info("Proxy Server started on port " + serverSocket.getLocalPort());

            while (true) {
                Socket requestSocket = serverSocket.accept();

                logger.info("Client connected, Full Socket: " + requestSocket);

                try (PrintWriter out = new PrintWriter(requestSocket.getOutputStream(), true);
                     BufferedReader in = new BufferedReader(new InputStreamReader(requestSocket.getInputStream()))) {
                    String input = in.readLine();

                    Pattern pattern = Pattern.compile("^(GET) (.*) HTTP/1.1$");
                    Matcher matcher = pattern.matcher(input);
                    if (!matcher.find()) {
                        out.println("HTTP/1.1 400 Bad Request");
                    } else {
                        String rawTargetUri = null;
                        try {
                            rawTargetUri = matcher.group(2);
                            logger.info("Target Host: " + rawTargetUri);
                        } catch (IllegalStateException | IndexOutOfBoundsException ex) {
                            // Should not happen, but just in case
                            logger.info(ex.toString());
                            requestSocket.close();
                            continue;
                        }

                        URI targetUri;
                        try {
                            targetUri = new URI(rawTargetUri);
                        } catch (URISyntaxException e) {
                            logger.info("Invalid URI: " + rawTargetUri);
                            out.println("HTTP/1.1 400 Bad Request");
                            requestSocket.close();
                            continue;
                        }

                        if (!"http".equals(targetUri.getScheme())) {
                            logger.info("Not an http host");
                            out.println("HTTP/1.1 400 Bad Request");
                            requestSocket.close();
                            continue;
                        }

                        // TODO detect port from request
                        logger.info("Connecting to socket " + targetUri.getHost() + ":80");
                        try (Socket targetSocket = new Socket(targetUri.getHost(), 80)) {
                            logger.info("Forwarding request to " + rawTargetUri);
                            try (PrintWriter targetHostWriter = new PrintWriter(targetSocket.getOutputStream(), true);
                                 BufferedReader targetHostReader = new BufferedReader(new InputStreamReader(targetSocket.getInputStream()))) {
                                targetHostWriter.println("GET " + targetUri.getPath() + " HTTP/1.1");
                                // TODO maybe forward the actual request completely
                                //  and add X-Forwarded-For header
                                targetHostWriter.println("Host: " + targetUri.getHost());
                                targetHostWriter.println("User-Agent: Java HTTP Proxy");
                                targetHostWriter.println("Accept: */*");
                                targetHostWriter.print("\r\n\r\n");
                                targetHostWriter.flush();

                                logger.info("Request sent to target host");
                                // TODO stream not return -1
                                targetHostReader.lines()
                                        .peek(System.out::println)
                                        .forEach(out::println);
                            }
                        } catch (UnknownHostException e) {
                            logger.info("Unknown host: " + e);
                            out.println("HTTP/1.1 404 Not Found");
                        } catch (IOException e) {
                            logger.info("IO Exception: " + e);
                            out.println("HTTP/1.1 500 Internal Server Error");
                        }
                    }
                }

                logger.info("Response sent to client");
                requestSocket.close();
            }


        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
