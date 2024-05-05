const express = require('express');
const app = express();
const bodyParser = require('body-parser');

app.use(bodyParser.json());

app.post('/your-endpoint', (req, res) => {

  console.log('Alınan JSON:', req.body);
  
  res.json({ message: 'JSON alındı!' });
});

const port = 8080;
app.listen(port, () => {
  console.log(`Sunucu ${port} numaralı portta çalışıyor`);
});
