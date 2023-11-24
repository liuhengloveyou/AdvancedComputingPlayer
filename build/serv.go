package main

import "net/http"

func main() {

	// self.send_header('Access-Control-Allow-Origin', '*')
	// self.send_header('Access-Control-Allow-Methods', '*')
	// self.send_header('Access-Control-Allow-Headers', '*')
	// self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')

	// self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
	// self.send_header('Cross-Origin-Opener-Policy', 'same-origin')

	http.HandleFunc("/", func(w http.ResponseWriter, req *http.Request) {
		if req.RequestURI == "/favicon.ico" {
			return
		}

	})

	http.ListenAndServe(":8080", nil)
}
