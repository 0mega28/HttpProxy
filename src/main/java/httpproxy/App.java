package httpproxy;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Hello world!
 */
public class App {
    private final static Logger logger = Logger.getLogger(App.class.getName());
    private final static ExecutorService executor = Executors.newVirtualThreadPerTaskExecutor();

    public static void main(String[] args) {
        // Java Server socket bind to port 8989
        try (ServerSocket serverSocket = new ServerSocket(8989)) {
            logger.info("Proxy Server started on port " + serverSocket.getLocalPort());

            while (true) {
                Socket requestSocket = serverSocket.accept();

                executor.submit(() -> handleRequest(requestSocket));
            }


        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private static void handleRequest(Socket requestSocket) {
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
                    return;
                }

                URI targetUri;
                try {
                    targetUri = new URI(rawTargetUri);
                } catch (URISyntaxException e) {
                    logger.info("Invalid URI: " + rawTargetUri);
                    out.println("HTTP/1.1 400 Bad Request");
                    requestSocket.close();
                    return;
                }

                if (!"http".equals(targetUri.getScheme())) {
                    logger.info("Not an http host");
                    out.println("HTTP/1.1 400 Bad Request");
                    requestSocket.close();
                    return;
                }

                // TODO detect port from request
                logger.info("Connecting to socket " + targetUri.getHost() + ":80");
                try (Socket targetSocket = new Socket(targetUri.getHost(), 80)) {
                    logger.info("Forwarding request to " + rawTargetUri);
                    try (BufferedReader targetHostReader = new BufferedReader(new InputStreamReader(targetSocket.getInputStream()))) {

                        // TODO maybe forward the actual request completely
                        //  and add X-Forwarded-For header
                        String request = "GET " + targetUri.getPath() + " HTTP/1.1" + "\r\n" +
                                         "Host: " + targetUri.getHost() + "\r\n" +
                                         "User-Agent: Java HTTP Proxy" + "\r\n" +
                                         "Accept: */*" + "\r\n" +
                                         "\r\n";
                        targetSocket.getOutputStream().write(request.getBytes());

                        logger.info("Request sent to target host");
                        // TODO stream not return -1

                        String line;
                        int contentLength = -1;
                        while ((line = targetHostReader.readLine()) != null && !line.isEmpty()) {
                            if (line.toLowerCase().contains("content-length")) {
                                String contentLengthStr = line.substring(line.indexOf(":") + 1).trim();
                                try {
                                    contentLength = Integer.parseInt(contentLengthStr);
                                } catch (NumberFormatException e) {
                                    contentLength = -1; // Invalid content length
                                    logger.info("Invalid Content-Length: " + contentLengthStr);
                                }
                            }
                            out.println(line);
                        }

                        // add an empty line to indicate the end of headers
                        out.println();
                        if (contentLength == 0) {
                        } else if (contentLength > 0) {
                            char[] buffer = new char[contentLength];
                            int bytesRead = targetHostReader.read(buffer, 0, contentLength);
                            if (bytesRead > 0) {
                                String responseBody = new String(buffer, 0, bytesRead);
                                out.print(responseBody);
                                out.flush();
                            }
                        } else {
                            // no content length, read until EOF
                            targetHostReader.lines().forEach(out::println);
                        }

                    }
                } catch (UnknownHostException e) {
                    logger.info("Unknown host: " + e);
                    out.println("HTTP/1.1 404 Not Found");
                } catch (IOException e) {
                    logger.info("IO Exception: " + e);
                    out.println("HTTP/1.1 500 Internal Server Error");
                }
            }
        } catch (IOException e) {
            logger.info("Error handling request: " + e);
        }

        logger.info("Response sent to client");
        try {
            requestSocket.close();
        } catch (IOException e) {
            logger.log(Level.INFO, e, () -> "Error closing request socket");
        }
    }
}
