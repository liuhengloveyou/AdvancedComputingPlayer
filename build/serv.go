package main

import (
	"io"
	"net/http"
	"net/url"
	"os"
)

func main() {

	// self.send_header('Access-Control-Allow-Origin', '*')
	// self.send_header('Access-Control-Allow-Methods', '*')
	// self.send_header('Access-Control-Allow-Headers', '*')
	// self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')

	http.HandleFunc("/", func(w http.ResponseWriter, req *http.Request) {
		if req.RequestURI == "/favicon.ico" {
			return
		}

		fileName, _ := url.QueryUnescape(req.RequestURI[1:])

		f, err := os.Open("./" + fileName)
		if err != nil {
			w.Write([]byte(err.Error()))
			return
		}

		_, err = f.Stat()
		if err != nil {
			w.Write([]byte(err.Error()))
			return
		}

		w.Header().Set("Cross-Origin-Embedder-Policy", "require-corp")
		w.Header().Set("Cross-Origin-Opener-Policy", "same-origin")

		f.Seek(0, 0)
		io.Copy(w, f)
	})

	http.ListenAndServe(":8080", nil)
}
