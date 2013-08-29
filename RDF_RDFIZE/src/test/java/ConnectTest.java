import static org.hamcrest.core.Is.is;
import static org.junit.Assume.assumeThat;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.RETURNS_SMART_NULLS;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.junit.Assert.*;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.RandomAccessFile;
import java.net.InetAddress;
import java.net.URL;
import java.sql.Connection;
import java.sql.SQLException;

import net.antidot.paf.Configuration;
import net.antidot.paf.Document;
import net.antidot.paf.Handle;
import net.antidot.protobuf.event.EventProtos.Level;
import net.antidot.protobuf.event.EventProtos.Type;
import net.antidot.protobuf.paf.layer.LayerType;
import net.antidot.sql.model.core.DriverType;
import net.antidot.sql.model.core.SQLConnector;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

/**
 * 
 */

/**
 * @author lm
 * 
 */
public class ConnectTest {

    private class SetLayerAnswer implements Answer<Boolean> {
	private String layerContent = null;

	private SetLayerAnswer() {
	}

	public Boolean answer(InvocationOnMock invocation) {
	    Object[] args = invocation.getArguments();
	    layerContent = (String) args[0];
	    return true;
	}
	
	public String getLayerContent() {
	    return layerContent;
	}
    }

    // MySQL Database TEST settings
    public static final String userName = "root";
    public static final String password = "62CRici";
    public static final DriverType driver = DriverType.MysqlDriver;
    public static final String url = "jdbc:mysql://localhost/testing?characterEncoding=utf8&sessionVariables=sql_mode='ANSI',storage_engine=InnoDB";
    public static final String testDbName = "testing";
    public static final String dbName = "mysql";

    /**
     * @throws java.lang.Exception
     */
    @Before
    public void setUp() throws Exception {
	final String hostName = InetAddress.getLocalHost().getHostName();
	assumeThat(hostName, is("doku"));

	Connection conn = SQLConnector.connect(userName, password, url, driver);
	SQLConnector.resetMySQLDatabase(conn, driver);
    }

    /**
     * @throws java.lang.Exception
     */
    @After
    public void tearDown() throws Exception {
    }

    /**
     * Test method for {@link Db2TripleFilter#generate()}.
     * 
     * @throws ClassNotFoundException
     * @throws IllegalAccessException
     * @throws InstantiationException
     * @throws SQLException
     */
    @Test
    public void testGenerate() throws Exception {
	Configuration mockConf = mock(Configuration.class, RETURNS_SMART_NULLS);
	when(mockConf.has_arg(anyString())).thenReturn(true);
	when(mockConf.get_string("rdb2rdf_mode")).thenReturn("DirectMapping");
	when(mockConf.get_string("driver")).thenReturn(driver.getDriverName());
	when(mockConf.get_string("login")).thenReturn(userName);
	when(mockConf.get_string("password")).thenReturn(password);
	when(mockConf.get_string("url")).thenReturn(url);
	when(mockConf.get_boolean("create_doc_with_base_uri")).thenReturn(true);
	when(mockConf.get_string("base_uri")).thenReturn("http://antidot.net/test/");
	when(mockConf.get_string("rdf_format")).thenReturn("TURTLE");
	when(mockConf.get_string("nativestore_path")).thenReturn(null);
	
	URL file = getClass().getResource("create.sql");		
	Connection conn = SQLConnector.connect(userName, password, url, driver);
	SQLConnector.updateDatabase(conn, file.getFile());

	SetLayerAnswer keepLayerContent = new SetLayerAnswer();	
	Document mockDoc = mock(Document.class);
	when(mockDoc.set_layer(anyString(),
		               any(LayerType.Type.class),
		               anyString(),
		               anyString(),
		               anyString())).then(keepLayerContent);
	
	Handle mockHandle = mock(Handle.class);
	final Answer<Object> boLogToStderr = new Answer<Object>() {
	        public Object answer(InvocationOnMock invocation) {
	            Object[] args = invocation.getArguments();
	            System.err.println("LOG to BO: " + args[1]);
	            return null;
	        }};
	doAnswer(boLogToStderr).when(mockHandle).log(any(Type.class), anyString(), any(Level.class), anyString());
	doAnswer(boLogToStderr).when(mockHandle).log(any(Type.class), anyString(), any(Level.class));
	
	when(mockHandle.new_document(anyString())).thenReturn(mockDoc);

	Db2TripleFilter filter = new Db2TripleFilter(mockConf, mockHandle);
	filter.init(); // May not throw anything	
	filter.generate(); // May not throw anything
	
	String builtData = keepLayerContent.getLayerContent().replaceAll("\\s","");
	assertNotNull(builtData);
	
	System.out.println("*************************");
	System.out.println(builtData);
	System.out.println("*************************");
		
	URL ref = getClass().getResource("result.n3");
	
	RandomAccessFile f = new RandomAccessFile(ref.getFile(), "r");
	byte[] b = new byte[(int)f.length()];
	f.read(b);
	f.close();
	String refData = new String(b);
	
	System.out.println("*************************");
	System.out.println(refData.toString());
	System.out.println("*************************");
	
	assertEquals(builtData, refData);
    }

}
