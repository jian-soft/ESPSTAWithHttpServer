function postData(data) {
    fetch("/echo", {
        method: "POST", // or 'PUT'
        headers: {
            "Content-Type": "application/json",
        },
        body: JSON.stringify(data),
        })
        .then((response) => response.text())
        .then((data) => {
            console.log("Success:", data);
        })
        .catch((error) => {
            console.error("Error:", error);
        });
}
